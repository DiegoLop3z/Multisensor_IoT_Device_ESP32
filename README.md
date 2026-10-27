# ESP32 Sistema Multisensor IoT con Servidor Web Embebido

Dispositivo IoT multisensor basado en **ESP32** para monitoreo ambiental y de presencia (**DHT11 / SHT41**, **PIR** y **LDR**), con **Hotspot Wi-Fi propio (SoftAP)** y **Servidor Web HTTP nativo** (`GET` / `POST` con interfaz **HTML + CSS** profesional), control de **módulo relé** con **confirmación de estado en lazo cerrado**, **4 modos de operación** e indicador físico mediante **LED RGB** y pantalla **OLED SSD1306**.

ESP32-based multisensor IoT device for environmental and occupancy monitoring (**DHT11 / SHT41**, **PIR**, and **LDR**) featuring a built-in **Wi-Fi SoftAP Hotspot** and a native **HTTP Web Server** (`GET` / `POST` with a clean **HTML + CSS** dashboard), **relay module** control with **closed-loop state confirmation**, **4 operating modes**, and physical mode indication via an **RGB LED** and **SSD1306 OLED** display.

---

## Problema que resuelve

Automatización y supervisión local de confort ambiental, iluminación y presencia en un espacio cerrado (laboratorio, oficina o habitación), sin depender de routers externos ni servidores adicionales: la propia **ESP32 genera su red Wi-Fi (Hotspot)** y aloja un **servidor web ligero** desde el cual cualquier computadora o teléfono conectado puede visualizar los sensores en tiempo real, elegir entre 4 modos de operación (Manual, Auto por Iluminación, Auto por Presencia o Auto por Clima) y controlar un módulo relé con confirmación del estado físico real.

---

## Arquitectura del Sistema

```text
Variables físicas (Temp/Hum, Luz, Movimiento)
  → Sensores (DHT11/SHT41 + LDR + PIR)
  → ESP32 (Máquina de Estados + Hotspot Wi-Fi SoftAP + Servidor Web HTTP GET/POST)
  ⇄ Actuadores Locales (Módulo Relé + LED RGB indicador de modo + OLED SSD1306)
  ⇄ Usuario (Dashboard Web HTML/CSS en http://192.168.4.1 desde PC o Smartphone)
```

![Arquitectura del sistema multisensor](docs/diagrama_arquitectura.png)

- **Variables físicas y sensores:**
  - **Temperatura (°C) y Humedad Relativa (%):** medidas por sensor digital **DHT11** (`GPIO 4`) y/o **Sensirion GY-SHT41** (I²C `0x44` en `GPIO 21`/`22`, heredado y validado de la práctica anterior).
  - **Nivel de iluminación (% y lectura ADC):** medido por **LDR (fotocelda)** en divisor de tensión conectado a `GPIO 34` (`ADC1_CH6`, compatible con Wi-Fi activo).
  - **Presencia / Movimiento:** detectado por sensor **PIR** (salida digital en `GPIO 35`).
- **Actuadores e indicadores físicos:**
  - **Módulo Relé (`GPIO 23`):** conmuta la salida de potencia según el modo activo o el comando manual, con lógica configurable (`RELAY_ACTIVE_LOW`).
  - **LED RGB (`GPIO 25` Rojo, `GPIO 26` Verde, `GPIO 27` Azul):** indica visualmente en todo momento el **modo de operación** seleccionado.
  - **Pantalla OLED 128×32 SSD1306 (I²C `0x3C`):** alterna las lecturas de los sensores cada 3 s y muestra el modo activo y el estado real del relé (`REL:ON` / `REL:OFF`).
- **Red Wi-Fi (Hotspot en la ESP32):**
  - La ESP32 levanta un punto de acceso Wi-Fi WPA2 propio (`ESP32-Multisensor`, IP fija `192.168.4.1`), con opción configurable para conectarse simultáneamente como estación (`WIFI_AP_STA`) a una red existente.
- **Servidor Web en la ESP32 (`WebServer.h`, puerto 80):**
  - Infraestructura ligera y directa usando la librería nativa `<WebServer.h>` de Arduino-ESP32.
  - Interfaz **HTML + CSS** limpia y responsiva almacenada en memoria Flash (`PROGMEM`), manteniendo la estética y el **control con confirmación de estado real** mediante peticiones `GET` y `POST`.

---

## Modos de Funcionamiento e Indicador LED RGB

El sistema cuenta con **4 modos de operación** seleccionables desde el dashboard web. El **LED RGB** cambia de color inmediatamente para indicar el modo activo:

| Modo | ID | Color LED RGB | Criterio de Decisión sobre el Módulo Relé |
|---|---|---|---|
| **Manual** | `MANUAL` (0) | 🔵 **Azul** `(0, 0, 255)` | El usuario controla el relé directamente (`ON` / `OFF`) mediante botones `POST` en la interfaz web. |
| **Auto Iluminación (LDR)** | `AUTO_LDR` (1) | 🟢 **Verde** `(0, 255, 0)` | Activa el relé automáticamente cuando la iluminación baja del umbral de oscuridad (`< 35%`) y lo apaga al superar `> 45%` (histéresis de 10% para evitar falsos disparos). |
| **Auto Presencia (PIR)** | `AUTO_PIR` (2) | 🟡 **Amarillo** `(255, 255, 0)` | Activa el relé al detectar movimiento en el sensor PIR y mantiene un tiempo de retención (*hold time* no bloqueante de `15 s`) tras el último pulso antes de apagarlo. |
| **Auto Ambiental (Clima)** | `AUTO_CLIMA` (3) | 🔴 **Rojo** `(255, 0, 0)` | Activa el relé (ventilación/extracción) si la temperatura `>= 28.0 °C` o humedad `>= 75.0 %`, y lo apaga cuando desciende a `<= 26.5 °C` y `< 70.0 %` (histéresis térmica). |

---

## Endpoints HTTP (`GET` y `POST`) del Servidor Web

| Ruta | Método | Parámetros / Respuesta | Descripción |
|---|---|---|---|
| `/` | `GET` | `text/html` (Dashboard HTML + CSS) | Página principal con tarjetas de sensores, estado del relé, color del LED RGB y controles de modo/relé. |
| `/status` | `GET` | `application/json` | Devuelve los valores actuales (`temp`, `hum`, `ldr_pct`, `ldr_state`, `pir`, `relay`, `mode`, `rgb`) para actualizar los datos en pantalla sin recargar la vista. |
| `/mode` | `POST` | `mode=0\|1\|2\|3` | Cambia el modo de operación en la ESP32, actualiza el LED RGB, evalúa la lógica del relé y devuelve el estado real confirmado. |
| `/relay` | `POST` | `state=ON\|OFF` | Conmuta físicamente el módulo relé (cuando el sistema está en modo `MANUAL`) y devuelve la confirmación del estado real del pin `GPIO 23`. |

**Control con confirmación de estado:** la interfaz web no asume que un comando se ejecutó solo por hacer clic; envía la petición `POST` a la ESP32, el microcontrolador aplica el cambio físico en el relé o LED RGB, y responde con el estado real confirmado para actualizar la interfaz.

---

## Cumplimiento de Requerimientos Funcionales (Práctica 2 - Unidad 2)

- [x] **1. Conectarse / proveer red Wi-Fi:** Hotspot Wi-Fi propio en la ESP32 (`WIFI_AP` / `WIFI_AP_STA`, IP `192.168.4.1`) para conexión directa desde laptop o celular.
- [x] **2. Implementar un servidor web en la ESP32:** Servidor HTTP nativo en puerto 80 (`WebServer.h`) con interfaz HTML + CSS y rutas `GET` / `POST`.
- [x] **3. Detectar movimiento mediante el sensor PIR:** Lectura digital continua en `GPIO 35` con ventana de retención no bloqueante (`millis()`).
- [x] **4. Detectar el nivel de iluminación mediante el LDR:** Lectura analógica de 12 bits (`0–4095`) en `GPIO 34` (`ADC1`), promedio móvil y conversión a porcentaje (`0–100%`) y estado (`OSCURO` / `MEDIO` / `ILUMINADO`).
- [x] **5. Obtener temperatura y humedad mediante el DHT11 (y soporte SHT41):** Lectura periódica cada 2 s sin bloquear el servidor web.
- [x] **6. Mostrar los valores de los sensores mediante la interfaz web:** Tarjetas claras para Temperatura, Humedad, Estado del PIR y Nivel del LDR.
- [x] **7. Controlar un módulo relé:** Conmutación digital en `GPIO 23` con histéresis en modos automáticos y control directo en modo manual.
- [x] **8. Permitir controlar el sistema mediante diferentes modos de funcionamiento:** 4 modos (`MANUAL`, `AUTO_LDR`, `AUTO_PIR`, `AUTO_CLIMA`) seleccionables vía `POST /mode`.
- [x] **9. Utilizar el LED RGB como indicador del modo seleccionado:** Salidas en `GPIO 25, 26, 27` con código de colores por modo.
- [x] **10. Permitir visualizar el estado actual del sistema desde un navegador web:** Acceso directo en `http://192.168.4.1/` desde cualquier navegador conectado al Hotspot de la ESP32.

---

## Estructura del Repositorio

```text
Multisensor_IoT_Device_ESP32/
├── firmware/
│   ├── esp32-multisensor-web/        → Firmware principal (Hotspot + Servidor Web HTML/CSS + Sensores + Relé + RGB)
│   └── esp32-scanner-i2c/            → Escáner I²C de diagnóstico (verifica OLED 0x3C y SHT41 0x44)
├── hardware/
│   ├── hardware_conexion.md          → Lista de materiales, tabla de pines y cálculos eléctricos
│   ├── diagrama_conexiones.svg       → Diagrama electrónico vectorial editable
│   └── diagrama_conexiones.png       → Diagrama electrónico en alta resolución
├── docs/
│   ├── documentacion_sistema_iot.md  → Explicación técnica (sensores, decisiones, relé, servidor web y modos)
│   ├── diagrama_arquitectura.svg     → Diagrama de arquitectura y flujo HTTP GET/POST
│   ├── diagrama_arquitectura.png     → Diagrama de arquitectura exportado
│   └── diagramas/                    → Scripts en Python generadores de los diagramas SVG/PNG
├── PLAN_IMPLEMENTACION.md            → Plan de implementación del proyecto
├── LICENSE                           → Licencia MIT
├── .gitignore                        → Exclusión de compilación, secretos y archivos temporales
└── README.md                         → Este archivo
```

---

## Materiales Requeridos y Asignación de Pines (Pinout)

| Componente | Especificación | Pin ESP32 | Notas Técnicas |
|---|---|---|---|
| **Microcontrolador** | ESP32 DevKit V1 (30 pines) | `3V3`, `VIN (5V)`, `GND` | Alimentación por cable USB de datos |
| **Sensor Temp/Humedad** | DHT11 (Digital 1-Wire) / GY-SHT41 (I²C) | `GPIO 4` (DHT11) / `GPIO 21, 22` (I²C) | Soporte configurable para DHT11 o SHT41 |
| **Sensor de Movimiento** | PIR HC-SR501 / AM312 | `GPIO 35` (Entrada digital) | Alimentación a 5V (`VIN`), salida lógica 3.3V |
| **Sensor de Iluminación** | LDR (Fotocelda) + Resistor 10 kΩ | `GPIO 34` (`ADC1_CH6`) | Divisor resistivo a 3.3V (`ADC1` funciona con Wi-Fi activo) |
| **Indicador de Modo** | LED RGB + 3× Resistores 220 Ω | `GPIO 25` (R), `GPIO 26` (G), `GPIO 27` (B) | Resistores limitadores de 220 Ω por canal |
| **Actuador de Salida** | Módulo Relé de 1 canal | `GPIO 23` | Alimentación `VIN` (5V) y control en `GPIO 23` |
| **Pantalla Local (Base)** | OLED 128×32 SSD1306 (I²C `0x3C`) | `GPIO 21` (SDA), `GPIO 22` (SCL) | Muestra lecturas, modo activo y estado del relé |

Detalle completo y diagrama electrónico en [`hardware/hardware_conexion.md`](hardware/hardware_conexion.md).

---

## Compilación y Uso Rápido (PlatformIO)

1. Copiar el archivo de configuración de red:
   ```bash
   cp firmware/esp32-multisensor-web/src/secrets.h.example firmware/esp32-multisensor-web/src/secrets.h
   ```
2. Compilar y subir el firmware a la ESP32:
   ```bash
   cd firmware/esp32-multisensor-web
   pio run -t upload
   pio device monitor
   ```
3. Conectarse desde la laptop o celular a la red Wi-Fi creada por la ESP32 (por defecto `ESP32-Multisensor`) y abrir en el navegador:
   - **Dashboard Web:** `http://192.168.4.1/`

---

## Autores

- **Diego López** — GitHub: [@DiegoLop3z](https://github.com/DiegoLop3z) (`diegolopez.inquire@hotmail.com`)
- **Arturo Heredia** — GitHub: [@arturoherediam](https://github.com/arturoherediam) (`arturoherediam.94@gmail.com`)

**Asignatura:** Internet de las Cosas (IoT) — Unidad 2, Práctica 2  
**Institución:** Universidad Politécnica de Yucatán (UPY) — Ciclo Escolar 2026  
**Docente:** M.I. Javier Poot

---

## Licencia

MIT — ver [LICENSE](LICENSE).
