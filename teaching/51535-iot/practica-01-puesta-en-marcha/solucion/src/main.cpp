// Practica 1 — solucion de referencia.
// La recta de ppm usa constantes de ejemplo (lab_config.h). Hay que
// sustituirlas por la medida del grupo antes de tratar el numero como dato.

#include <Arduino.h>
#include <WiFi.h>

#include "lab_config.h"

constexpr int PIN_SENSOR = 34;
constexpr int PIN_LED = 2;
constexpr uint32_t PERIODO_MS = 2000;
constexpr uint32_t WIFI_TIMEOUT_MS = 15000;

float adcToVolts(int adc) {
  return (static_cast<float>(adc) * LAB_ADC_VREF) / LAB_ADC_FULL_SCALE;
}

float estimarPpm(int adc, float volts) {
  (void)volts;
  return LAB_PPM_AIRE + (static_cast<float>(adc) - LAB_ADC_AIRE) * LAB_PPM_POR_CUENTA;
}

bool conectarWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(LAB_WIFI_SSID, LAB_WIFI_PASSWORD);
  Serial.printf("Conectando a %s", LAB_WIFI_SSID);
  const uint32_t inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - inicio > WIFI_TIMEOUT_MS) {
      Serial.println("\nWiFi: tiempo agotado. Revisa include/secrets.h.");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi conectado. IP %s  RSSI %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  return true;
}

void parpadeoInicial() {
  for (int i = 0; i < 3; ++i) {
    digitalWrite(PIN_LED, HIGH);
    delay(150);
    digitalWrite(PIN_LED, LOW);
    delay(150);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(PIN_LED, OUTPUT);
  analogReadResolution(12);
  Serial.println("Practica 1 — solucion. Constantes de ppm de ejemplo.");
  parpadeoInicial();
  conectarWifi();
}

void loop() {
  const int adc = analogRead(PIN_SENSOR);
  const float volts = adcToVolts(adc);
  const float ppm = estimarPpm(adc, volts);
  const int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  Serial.printf("adc=%d  V=%.3f  ppm=%.1f  rssi=%d\n", adc, volts, ppm, rssi);
  digitalWrite(PIN_LED, !digitalRead(PIN_LED));
  delay(PERIODO_MS);
}
