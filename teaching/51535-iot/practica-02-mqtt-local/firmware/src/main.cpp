// Practica 2 — MQTT local. Codigo de partida.
// Publica el ADC en JSON minimo y un status retenido.
// Completa los TODO: ppm, campos del enunciado antiguo y comando del LED.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <cstdio>
#include <cstring>

#include "lab_config.h"

constexpr int PIN_SENSOR = 34;
constexpr int PIN_LED = 2;
constexpr uint32_t PERIODO_MS = 5000;
constexpr uint32_t WIFI_TIMEOUT_MS = 15000;

WiFiClient net;
PubSubClient mqtt(net);

char topic_co2[64];
char topic_status[64];
char topic_led[64];
char client_id[32];

void buildNames() {
  snprintf(topic_co2, sizeof(topic_co2), "lab/51535/%s/co2", LAB_GROUP_ID);
  snprintf(topic_status, sizeof(topic_status), "lab/51535/%s/status", LAB_GROUP_ID);
  snprintf(topic_led, sizeof(topic_led), "lab/51535/%s/cmd/led", LAB_GROUP_ID);
  snprintf(client_id, sizeof(client_id), "lab51535-%s", LAB_GROUP_ID);
}

float adcToVolts(int adc) {
  return (static_cast<float>(adc) * LAB_ADC_VREF) / LAB_ADC_FULL_SCALE;
}

// TODO(estudiante): devuelve el ppm de la practica 1. Hoy el valor publicado
// es la cuenta cruda, para que veas el broker antes de cerrar la conversion.
float estimarPpm(int adc, float volts) {
  (void)volts;
  return static_cast<float>(adc);
}

void onMqtt(char* topic, uint8_t* payload, unsigned int len) {
  (void)topic;
  (void)payload;
  (void)len;
  // TODO(estudiante): si topic es topic_led, lee {"on": true/false}
  // y pon PIN_LED en HIGH o LOW.
}

bool conectarWifi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.begin(LAB_WIFI_SSID, LAB_WIFI_PASSWORD);
  Serial.printf("Conectando a %s", LAB_WIFI_SSID);
  const uint32_t inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - inicio > WIFI_TIMEOUT_MS) {
      Serial.println("\nWiFi: tiempo agotado.");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi conectado. IP %s\n", WiFi.localIP().toString().c_str());
  return true;
}

void reconnect() {
  // Bloquea a proposito. La practica 4 lo convierte en un intento periodico.
  while (!mqtt.connected()) {
    if (WiFi.status() != WL_CONNECTED) return;
    Serial.print("MQTT...");
    if (mqtt.connect(client_id)) {
      Serial.println("ok");
      // TODO(estudiante): mqtt.subscribe(topic_led, 1);
      return;
    }
    Serial.printf(" rc=%d, reintento en 5 s\n", mqtt.state());
    delay(5000);
  }
}

void publishCo2(int adc, float ppm) {
  JsonDocument doc;
  // TODO(estudiante): anade sensor_id, tipo, y que "valor" sea el ppm.
  doc["valor"] = adc;
  (void)ppm;
  char buf[192];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(topic_co2, buf);
  Serial.printf("publicado %s %s\n", topic_co2, buf);
}

void publishStatus() {
  JsonDocument doc;
  doc["state"] = "RUN";
  doc["pwm"] = 0.0;
  doc["fan"] = false;
  doc["uptime_s"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();
  char buf[192];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(topic_status, buf, /*retain=*/true);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(PIN_LED, OUTPUT);
  analogReadResolution(12);
  buildNames();
  conectarWifi();
  mqtt.setServer(LAB_MQTT_HOST, LAB_MQTT_PORT);
  mqtt.setCallback(onMqtt);
  mqtt.setBufferSize(512);
  Serial.printf("cliente %s -> %s:%d\n", client_id, LAB_MQTT_HOST, LAB_MQTT_PORT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectarWifi();
  if (!mqtt.connected()) reconnect();
  mqtt.loop();

  const int adc = analogRead(PIN_SENSOR);
  const float ppm = estimarPpm(adc, adcToVolts(adc));
  if (mqtt.connected()) {
    publishCo2(adc, ppm);
    publishStatus();
  }
  delay(PERIODO_MS);
}
