#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"
#include "web_dashboard.h"

// ============================================================================
//  Practica 2 (Unidad 2) — Sistema Multisensor con Servidor Web ESP32
//
//  - Mantiene los mismos pines I2C (GPIO 21 SDA / GPIO 22 SCL) de la practica
//    anterior para el sensor GY-SHT41 (0x44) y la pantalla OLED SSD1306 (0x3C),
//    con soporte automatico de respaldo para sensor DHT11 en GPIO 4.
//  - Lee iluminacion con LDR en GPIO 34 (ADC1, compatible con Wi-Fi activo).
//  - Detecta movimiento con sensor PIR en GPIO 35 (con retencion no bloqueante).
//  - 4 Modos de operacion indicados por LED RGB (GPIO 25, 26, 27):
//      0: MANUAL      -> LED Azul     (control del rele desde botones web)
//      1: AUTO_LDR    -> LED Verde    (rele ON si luz < 35%, OFF si luz > 45%)
//      2: AUTO_PIR    -> LED Amarillo (rele ON con presencia PIR + 15s hold)
//      3: AUTO_CLIMA  -> LED Rojo     (rele ON si Temp >= 28 C o Hum >= 75%)
//  - ESP32 como Hotspot Wi-Fi propio (192.168.4.1) + Servidor Web HTTP (puerto 80)
//    con peticiones GET y POST y confirmacion del estado real del rele.
// ============================================================================

// ---------- Pines de I2C (identicos a la practica anterior) ----------
#define SDA_PIN 21
#define SCL_PIN 22
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// ---------- Pines de Sensores Nuevos ----------
#define DHT_PIN 4          // Respaldo DHT11 si no se detecta el SHT41 en I2C
#define DHT_TYPE DHT11
#define LDR_PIN 34         // ADC1_CH6 (funciona con Wi-Fi activo sin conflicto)
#define PIR_PIN 35         // Entrada digital del sensor PIR

// ---------- Pines de Actuadores (Rele y LED RGB) ----------
#define RELAY_PIN 23       // Pin de control del modulo rele
#define LED_ONBOARD_PIN 2  // LED azul integrado del DevKit (espejo visual del rele)
#define RGB_R_PIN 25       // Canal Rojo del LED RGB (con resistencia 220 ohm)
#define RGB_G_PIN 26       // Canal Verde del LED RGB (con resistencia 220 ohm)
#define RGB_B_PIN 27       // Canal Azul del LED RGB (con resistencia 220 ohm)

// ---------- Configuracion de Polaridad de Hardware ----------
// Cambiar RELAY_ACTIVE_LOW a true si tu modulo rele se activa con nivel BAJO (0V)
const bool RELAY_ACTIVE_LOW = false;
// Cambiar RGB_COMMON_ANODE a true si tu LED RGB es de anodo comun (3.3V comun)
const bool RGB_COMMON_ANODE = false;
// Cambiar LDR_INVERTED a true si con mas luz el voltaje del divisor disminuye
const bool LDR_INVERTED = false;

// ---------- Modos de Operacion ----------
enum ModoOperacion : uint8_t {
  MODO_MANUAL     = 0,  // Azul: control directo desde la interfaz web
  MODO_AUTO_LDR   = 1,  // Verde: control automatico por nivel de iluminacion
  MODO_AUTO_PIR   = 2,  // Amarillo: control automatico por movimiento PIR
  MODO_AUTO_CLIMA = 3   // Rojo: control automatico por temperatura/humedad
};

// ---------- Umbrales de Decision e Histeresis ----------
const int   LDR_UMBRAL_ENCENDIDO_PCT = 35;    // < 35% (oscuro) -> Enciende rele
const int   LDR_UMBRAL_APAGADO_PCT   = 45;    // > 45% (iluminado) -> Apaga rele
const float TEMP_UMBRAL_ENCENDIDO_C  = 28.0;  // >= 28.0 C -> Enciende rele
const float TEMP_UMBRAL_APAGADO_C    = 26.5;  // <= 26.5 C -> Apaga rele
const float HUM_UMBRAL_ENCENDIDO_PCT = 75.0;  // >= 75.0 % -> Enciende rele
const float HUM_UMBRAL_APAGADO_PCT   = 70.0;  // <  70.0 % -> Apaga rele

// ---------- Intervalos No Bloqueantes (millis) ----------
const unsigned long INTERVALO_TEMP_HUM_MS     = 2000;  // Leer SHT41/DHT11 cada 2 s
const unsigned long INTERVALO_LDR_MS          = 250;   // Muestrear LDR cada 250 ms
const unsigned long INTERVALO_OLED_MS         = 2500;  // Rotar pantalla OLED cada 2.5 s
const unsigned long TIEMPO_RETENCION_PIR_MS   = 15000; // Retener PIR activo por 15 s

// ---------- Instancias de Hardware y Servidor Web ----------
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);

// ---------- Variables de Estado del Sistema ----------
bool sht41Disponible = false;
bool oledDisponible  = false;

float temperaturaActual = 25.0;
float humedadActual     = 55.0;

// Filtro de media movil para el ADC del LDR (8 muestras)
const uint8_t LDR_MUESTRAS = 8;
int bufferLdr[LDR_MUESTRAS] = {0};
uint8_t indiceLdr = 0;
bool bufferLdrLleno = false;
int ldrCrudo = 0;
int ldrPorcentaje = 50;
const char* ldrEstadoTexto = "MEDIO";

// Estado del sensor PIR
bool pirPinActual = false;
bool movimientoActivo = false;
unsigned long ultimoMovimientoMs = 0;

// Estado de Modo y Rele (confirmados)
ModoOperacion modoActual = MODO_MANUAL;
bool releEncendido = false;

// Temporizadores millis()
unsigned long ultimaLecturaTempHum = 0;
unsigned long ultimaLecturaLdr     = 0;
unsigned long ultimoCambioOled     = 0;
uint8_t paginaOled = 0;

// ============================================================================
//  Funciones Auxiliares de Nombres y Colores
// ============================================================================

const char* nombreModo(ModoOperacion m) {
  switch (m) {
    case MODO_MANUAL:     return "MANUAL";
    case MODO_AUTO_LDR:   return "AUTO_LDR";
    case MODO_AUTO_PIR:   return "AUTO_PIR";
    case MODO_AUTO_CLIMA: return "AUTO_CLIMA";
    default:              return "MANUAL";
  }
}

const char* nombreCortoModo(ModoOperacion m) {
  switch (m) {
    case MODO_MANUAL:     return "M:MAN";
    case MODO_AUTO_LDR:   return "M:LDR";
    case MODO_AUTO_PIR:   return "M:PIR";
    case MODO_AUTO_CLIMA: return "M:CLI";
    default:              return "M:MAN";
  }
}

const char* colorHexModo(ModoOperacion m) {
  switch (m) {
    case MODO_MANUAL:     return "#0066ff"; // Azul
    case MODO_AUTO_LDR:   return "#16a34a"; // Verde
    case MODO_AUTO_PIR:   return "#eab308"; // Amarillo
    case MODO_AUTO_CLIMA: return "#dc2626"; // Rojo
    default:              return "#0066ff";
  }
}

// ============================================================================
//  Pantalla OLED SSD1306 (0x3C en GPIO 21/22)
// ============================================================================

void actualizarOled() {
  if (!oledDisponible) return;

  display.clearDisplay();

  // Barra superior fija: Modo activo a la izquierda, estado del Rele a la derecha
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(nombreCortoModo(modoActual));

  const char* txtRele = releEncendido ? "REL:ON" : "REL:OFF";
  int anchoRele = strlen(txtRele) * 6;
  display.setCursor(SCREEN_WIDTH - anchoRele, 0);
  display.print(txtRele);

  // Linea principal (rota entre 3 vistas cada 2.5 s)
  display.setCursor(0, 14);
  if (paginaOled == 0) {
    display.setTextSize(2);
    display.print(temperaturaActual, 1);
    display.print(" C");
  } else if (paginaOled == 1) {
    display.setTextSize(2);
    display.print(humedadActual, 1);
    display.print(" %");
  } else {
    display.setTextSize(1);
    display.print("Luz: ");
    display.print(ldrPorcentaje);
    display.print("% (");
    display.print(ldrEstadoTexto);
    display.println(")");
    display.setCursor(0, 24);
    display.print("PIR: ");
    display.print(movimientoActivo ? "MOVIMIENTO" : "LIBRE");
  }

  display.display();
}

// ============================================================================
//  Control de Actuadores: LED RGB y Modulo Rele
// ============================================================================

void escribirCanalRgb(uint8_t pin, bool encender) {
  bool nivelAlto = RGB_COMMON_ANODE ? !encender : encender;
  digitalWrite(pin, nivelAlto ? HIGH : LOW);
}

// Actualiza los pines fisicos del LED RGB segun el modo seleccionado
void actualizarLedRgb(ModoOperacion modo) {
  switch (modo) {
    case MODO_MANUAL:     // Azul (0, 0, 1)
      escribirCanalRgb(RGB_R_PIN, false);
      escribirCanalRgb(RGB_G_PIN, false);
      escribirCanalRgb(RGB_B_PIN, true);
      break;
    case MODO_AUTO_LDR:   // Verde (0, 1, 0)
      escribirCanalRgb(RGB_R_PIN, false);
      escribirCanalRgb(RGB_G_PIN, true);
      escribirCanalRgb(RGB_B_PIN, false);
      break;
    case MODO_AUTO_PIR:   // Amarillo = Rojo + Verde (1, 1, 0)
      escribirCanalRgb(RGB_R_PIN, true);
      escribirCanalRgb(RGB_G_PIN, true);
      escribirCanalRgb(RGB_B_PIN, false);
      break;
    case MODO_AUTO_CLIMA: // Rojo (1, 0, 0)
      escribirCanalRgb(RGB_R_PIN, true);
      escribirCanalRgb(RGB_G_PIN, false);
      escribirCanalRgb(RGB_B_PIN, false);
      break;
  }
}

// Conmuta el modulo rele fisico y actualiza el estado real confirmado
void aplicarRele(bool encender) {
  bool cambio = (releEncendido != encender);
  releEncendido = encender;

  bool nivelPin = RELAY_ACTIVE_LOW ? !encender : encender;
  digitalWrite(RELAY_PIN, nivelPin ? HIGH : LOW);
  digitalWrite(LED_ONBOARD_PIN, encender ? HIGH : LOW);

  if (cambio) {
    Serial.print("[ACTUADOR] Modulo Rele -> ");
    Serial.print(encender ? "ON" : "OFF");
    Serial.print(" (Modo: ");
    Serial.print(nombreModo(modoActual));
    Serial.println(")");
    actualizarOled();
  }
}

// ============================================================================
//  Logica de Toma de Decisiones (4 Modos de Operacion)
// ============================================================================

void evaluarLogicaControl() {
  switch (modoActual) {
    case MODO_MANUAL:
      // En Modo Manual el rele conserva el estado elegido por el usuario en la web
      break;

    case MODO_AUTO_LDR:
      // Histeresis luminica: prende con oscuridad (< 35%), apaga con luz (> 45%)
      if (!releEncendido && ldrPorcentaje < LDR_UMBRAL_ENCENDIDO_PCT) {
        aplicarRele(true);
      } else if (releEncendido && ldrPorcentaje > LDR_UMBRAL_APAGADO_PCT) {
        aplicarRele(false);
      }
      break;

    case MODO_AUTO_PIR:
      // Se activa mientras exista movimiento detectado (incluyendo ventana de 15 s)
      aplicarRele(movimientoActivo);
      break;

    case MODO_AUTO_CLIMA:
      // Histeresis termica/ambiental: activa si Temp >= 28 C o Hum >= 75%
      if (!releEncendido &&
          (temperaturaActual >= TEMP_UMBRAL_ENCENDIDO_C ||
           humedadActual >= HUM_UMBRAL_ENCENDIDO_PCT)) {
        aplicarRele(true);
      } else if (releEncendido &&
                 (temperaturaActual <= TEMP_UMBRAL_APAGADO_C &&
                  humedadActual < HUM_UMBRAL_APAGADO_PCT)) {
        aplicarRele(false);
      }
      break;
  }
}

void seleccionarModo(ModoOperacion nuevoModo) {
  modoActual = nuevoModo;
  actualizarLedRgb(modoActual);
  Serial.print("[MODO] Modo cambiado a: ");
  Serial.println(nombreModo(modoActual));
  evaluarLogicaControl();
  actualizarOled();
}

// ============================================================================
//  Adquisicion y Procesamiento de Sensores
// ============================================================================

void leerTemperaturaHumedad() {
  if (sht41Disponible) {
    sensors_event_t hum_ev, temp_ev;
    if (sht4.getEvent(&hum_ev, &temp_ev)) {
      temperaturaActual = temp_ev.temperature;
      humedadActual     = hum_ev.relative_humidity;
    } else {
      Serial.println("[WARN] Error leyendo SHT41 en bus I2C.");
    }
  } else {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temperaturaActual = t;
      humedadActual     = h;
    } else {
      Serial.println("[WARN] Sin respuesta valida del DHT11 en GPIO 4.");
    }
  }
}

void leerSensorLdr() {
  int muestra = analogRead(LDR_PIN);
  bufferLdr[indiceLdr] = muestra;
  indiceLdr = (indiceLdr + 1) % LDR_MUESTRAS;
  if (indiceLdr == 0) bufferLdrLleno = true;

  uint8_t totalMuestras = bufferLdrLleno ? LDR_MUESTRAS : (indiceLdr > 0 ? indiceLdr : 1);
  long suma = 0;
  for (uint8_t i = 0; i < totalMuestras; i++) {
    suma += bufferLdr[i];
  }
  ldrCrudo = suma / totalMuestras;

  int pct = round((ldrCrudo / 4095.0f) * 100.0f);
  pct = constrain(pct, 0, 100);
  if (LDR_INVERTED) pct = 100 - pct;
  ldrPorcentaje = pct;

  if (ldrPorcentaje < 35) {
    ldrEstadoTexto = "OSCURO";
  } else if (ldrPorcentaje <= 70) {
    ldrEstadoTexto = "MEDIO";
  } else {
    ldrEstadoTexto = "ILUMINADO";
  }
}

void leerSensorPir(unsigned long ahora) {
  pirPinActual = (digitalRead(PIR_PIN) == HIGH);
  if (pirPinActual) {
    ultimoMovimientoMs = ahora;
    if (!movimientoActivo) {
      movimientoActivo = true;
      Serial.println("[PIR] Movimiento detectado.");
      evaluarLogicaControl();
      actualizarOled();
    }
  } else if (movimientoActivo && (ahora - ultimoMovimientoMs >= TIEMPO_RETENCION_PIR_MS)) {
    movimientoActivo = false;
    Serial.println("[PIR] Fin de ventana de movimiento (libre).");
    evaluarLogicaControl();
    actualizarOled();
  }
}

// ============================================================================
//  Servidor Web HTTP (GET y POST en Puerto 80)
// ============================================================================

String construirJsonEstado() {
  char json[340];
  snprintf(json, sizeof(json),
           "{\"temp\":%.1f,\"hum\":%.1f,\"sensor\":\"%s\","
           "\"ldr_raw\":%d,\"ldr_pct\":%d,\"ldr_state\":\"%s\","
           "\"pir\":%s,\"relay\":%s,\"mode\":%u,\"mode_name\":\"%s\","
           "\"rgb\":\"%s\",\"uptime\":%lu}",
           temperaturaActual,
           humedadActual,
           sht41Disponible ? "SHT41 (I2C)" : "DHT11",
           ldrCrudo,
           ldrPorcentaje,
           ldrEstadoTexto,
           movimientoActivo ? "true" : "false",
           releEncendido ? "true" : "false",
           (unsigned int)modoActual,
           nombreModo(modoActual),
           colorHexModo(modoActual),
           millis() / 1000UL);
  return String(json);
}

// GET / -> Sirve la interfaz HTML + CSS
void handleRootGet() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

// GET /status -> Devuelve el estado actual de sensores y actuadores en JSON
void handleStatusGet() {
  server.send(200, "application/json", construirJsonEstado());
}

// POST /mode -> Cambia el modo de operacion (0..3) y confirma el estado real
void handleModePost() {
  if (!server.hasArg("mode")) {
    server.send(400, "application/json", "{\"error\":\"Falta parametro mode\"}");
    return;
  }
  int m = server.arg("mode").toInt();
  if (m < 0 || m > 3) {
    server.send(400, "application/json", "{\"error\":\"Modo invalido (use 0..3)\"}");
    return;
  }
  seleccionarModo((ModoOperacion)m);
  server.send(200, "application/json", construirJsonEstado());
}

// POST /relay -> Enciende o apaga el rele en Modo Manual y confirma el estado
void handleRelayPost() {
  if (modoActual != MODO_MANUAL) {
    server.send(409, "application/json",
                "{\"error\":\"Cambie a Modo Manual (0) para controlar el rele manualmente\"}");
    return;
  }
  if (!server.hasArg("state")) {
    server.send(400, "application/json", "{\"error\":\"Falta parametro state (ON/OFF)\"}");
    return;
  }
  String st = server.arg("state");
  st.trim();
  st.toUpperCase();
  if (st == "ON" || st == "1") {
    aplicarRele(true);
  } else if (st == "OFF" || st == "0") {
    aplicarRele(false);
  } else {
    server.send(400, "application/json", "{\"error\":\"Valor state invalido (use ON u OFF)\"}");
    return;
  }
  server.send(200, "application/json", construirJsonEstado());
}

void configurarServidorWeb() {
  server.on("/", HTTP_GET, handleRootGet);
  server.on("/status", HTTP_GET, handleStatusGet);
  server.on("/mode", HTTP_POST, handleModePost);
  server.on("/relay", HTTP_POST, handleRelayPost);
  server.begin();
  Serial.println("[WEB] Servidor HTTP iniciado en puerto 80.");
}

// ============================================================================
//  setup / loop
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n====================================================");
  Serial.println("  ESP32 Sistema Multisensor con Servidor Web (U2P2)");
  Serial.println("====================================================");

  // 1. Configurar pines de salida (Rele y LED RGB)
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_ONBOARD_PIN, OUTPUT);
  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);

  aplicarRele(false);
  actualizarLedRgb(MODO_MANUAL);

  // 2. Configurar pines de entrada (PIR y LDR en ADC1)
  pinMode(PIR_PIN, INPUT);
  analogReadResolution(12); // 0..4095 en GPIO 34

  // 3. Inicializar Bus I2C (mismos pines GPIO 21 y 22 de la practica anterior)
  Wire.begin(SDA_PIN, SCL_PIN);

  // 4. Inicializar Pantalla OLED SSD1306 (0x3C)
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    oledDisponible = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ESP32 Multisensor");
    display.println("Hotspot: 192.168.4.1");
    display.display();
    Serial.println("[OK] Pantalla OLED SSD1306 detectada en 0x3C.");
  } else {
    Serial.println("[INFO] OLED no detectada en 0x3C (el sistema continua normal).");
  }

  // 5. Inicializar Sensor de Temperatura/Humedad (SHT41 en 0x44 o DHT11 en GPIO 4)
  if (sht4.begin()) {
    sht41Disponible = true;
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
    Serial.println("[OK] Sensor GY-SHT41 detectado en I2C 0x44.");
  } else {
    dht.begin();
    Serial.println("[INFO] SHT41 no detectado; usando DHT11 en GPIO 4.");
  }

  // 6. Configurar Hotspot Wi-Fi en la ESP32 (SoftAP)
#if WIFI_STA_ENABLED
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#else
  WiFi.mode(WIFI_AP);
#endif

  bool apOk = WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress ipHotspot = WiFi.softAPIP();
  Serial.println("----------------------------------------------------");
  Serial.print("[WIFI] Hotspot activo: ");
  Serial.println(apOk ? "SI" : "ERROR");
  Serial.print("[WIFI] Red (SSID):     ");
  Serial.println(AP_SSID);
  Serial.print("[WIFI] Contrasena:     ");
  Serial.println(AP_PASSWORD);
  Serial.print("[WEB]  Panel Web URL:  http://");
  Serial.println(ipHotspot);
  Serial.println("----------------------------------------------------");

  // 7. Lectura inicial de sensores y arranque del Servidor Web
  leerSensorLdr();
  leerTemperaturaHumedad();
  actualizarOled();
  configurarServidorWeb();
}

void loop() {
  unsigned long ahora = millis();

  // 1. Atender clientes HTTP del navegador sin bloqueo
  server.handleClient();

  // 2. Monitorear sensor PIR continuamente (con retencion de 15 s)
  leerSensorPir(ahora);

  // 3. Muestrear iluminacion LDR cada 250 ms
  if (ahora - ultimaLecturaLdr >= INTERVALO_LDR_MS) {
    ultimaLecturaLdr = ahora;
    leerSensorLdr();
    evaluarLogicaControl();
  }

  // 4. Leer Temperatura y Humedad cada 2000 ms
  if (ahora - ultimaLecturaTempHum >= INTERVALO_TEMP_HUM_MS) {
    ultimaLecturaTempHum = ahora;
    leerTemperaturaHumedad();
    evaluarLogicaControl();

    Serial.printf("[TELEMETRIA] Temp: %.1f C | Hum: %.1f %% | LDR: %d %% (%s) | PIR: %s | Modo: %s | Rele: %s\n",
                  temperaturaActual, humedadActual, ldrPorcentaje, ldrEstadoTexto,
                  movimientoActivo ? "MOV" : "LIBRE", nombreModo(modoActual),
                  releEncendido ? "ON" : "OFF");
  }

  // 5. Rotar informacion en la pantalla OLED cada 2.5 s
  if (ahora - ultimoCambioOled >= INTERVALO_OLED_MS) {
    ultimoCambioOled = ahora;
    paginaOled = (paginaOled + 1) % 3;
    actualizarOled();
  }
}

