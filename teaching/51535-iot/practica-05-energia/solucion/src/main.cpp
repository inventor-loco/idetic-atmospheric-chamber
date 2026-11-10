// Practica 5 — solucion de referencia.
// Un ciclo: leer, publicar, soltar la radio, deep sleep.
// Si no hay enlace, duerme igual. boot_count vive en RTC.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include <cstdio>

#include "lab_config.h"

constexpr int PIN_SENSOR = 34;
constexpr uint32_t WIFI_TIMEOUT_MS = 12000;

RTC_DATA_ATTR uint32_t boot_count = 0;
RTC_DATA_ATTR uint32_t fail_count = 0;

WiFiClient net;
PubSubClient mqtt(net);
char topic_co2[64];
char client_id[32];

float estimarPpm(int adc) {
  return LAB_PPM_AIRE + (static_cast<float>(adc) - LAB_ADC_AIRE) * LAB_PPM_POR_CUENTA;
}

bool esperarWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(LAB_WIFI_SSID, LAB_WIFI_PASSWORD);
  const uint32_t inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - inicio > WIFI_TIMEOUT_MS) return false;
    delay(250);
  }
  return true;
}

void publicar(int adc, float ppm, uint32_t awake_ms) {
  JsonDocument doc;
  doc["sensor_id"] = LAB_GROUP_ID;
  doc["tipo"] = "CO2";
  doc["valor"] = ppm;
  doc["adc"] = adc;
  doc["seq"] = boot_count;
  doc["awake_ms"] = awake_ms;
  doc["sleep_s"] = LAB_SLEEP_S;
  doc["ts"] = millis() / 1000;
  char buf[256];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(topic_co2, buf);
  Serial.println(buf);
}

void dormir(uint32_t seconds) {
  if (seconds == 0) seconds = 60;
  if (mqtt.connected()) mqtt.disconnect();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(20);
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(seconds) * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  boot_count++;
  analogReadResolution(12);
  snprintf(topic_co2, sizeof(topic_co2), "lab/51535/%s/co2", LAB_GROUP_ID);
  snprintf(client_id, sizeof(client_id), "lab51535-%s", LAB_GROUP_ID);
  mqtt.setServer(LAB_MQTT_HOST, LAB_MQTT_PORT);
  mqtt.setBufferSize(512);
  Serial.printf("arranque=%lu fallos=%lu\n",
                static_cast<unsigned long>(boot_count),
                static_cast<unsigned long>(fail_count));
}

void loop() {
  const uint32_t t0 = millis();
  const bool wifi = esperarWifi();
  bool published = false;
  if (wifi && mqtt.connect(client_id)) {
    const int adc = analogRead(PIN_SENSOR);
    const uint32_t awake_ms = millis() - t0;
    publicar(adc, estimarPpm(adc), awake_ms);
    mqtt.loop();
    delay(100);
    published = true;
  }
  if (!published) {
    fail_count++;
    Serial.printf("sin publicar, fallos=%lu\n", static_cast<unsigned long>(fail_count));
  }
  dormir(LAB_SLEEP_S);
}
