#include <Arduino.h>
#include <Wire.h>

// Pines I2C por defecto del ESP32 DevKit
#define SDA_PIN 21
#define SCL_PIN 22

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println();
  Serial.println("Escaner I2C iniciado.");
}

void loop() {
  byte error, address;
  int dispositivos_encontrados = 0;

  Serial.println("Escaneando bus I2C...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Dispositivo encontrado en direccion 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      dispositivos_encontrados++;
    }
  }

  if (dispositivos_encontrados == 0) {
    Serial.println("No se encontraron dispositivos I2C. Revisa las conexiones.");
  } else {
    Serial.print(dispositivos_encontrados);
    Serial.println(" dispositivo(s) encontrado(s).");
  }

  Serial.println();
  delay(3000);
}