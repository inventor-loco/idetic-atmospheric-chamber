// Practica 3 — codigo de partida.
// Enumera ROM, lee por indice y publica temps sin offset.
// El indice de 1-Wire puede cambiar de un arranque a otro: no lo uses
// como identidad de la sonda. Completa los TODO.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <cstdio>

#include "lab_config.h"

constexpr int PIN_BUS_CTRL = 4;  // pins::CTRL_SENSOR_1WIRE. No se llama PIN_CTRL: ese nombre es un registro del ESP32.
constexpr int PIN_ARRAY = 5;  // pins::ARRAY_1WIRE
constexpr int ARRAY_COUNT = 5;
constexpr uint32_t CONVERSION_MS = 750;
constexpr uint32_t PERIODO_MS = 2000;
constexpr uint32_t WIFI_TIMEOUT_MS = 15000;

OneWire ctrlWire(PIN_BUS_CTRL);
OneWire arrayWire(PIN_ARRAY);
DallasTemperature ctrlBus(&ctrlWire);
DallasTemperature arrayBus(&arrayWire);

WiFiClient net;
PubSubClient mqtt(net);
char topic_temps[64];
char client_id[32];

void printRom(const uint8_t* rom) {
  for (int i = 0; i < 8; ++i) {
    if (i) Serial.print(':');
    if (rom[i] < 16) Serial.print('0');
    Serial.print(rom[i], HEX);
  }
}

// TODO(estudiante): localiza el indice cuyo ROM coincida con el de vuestra
// tabla. Devolver `fallback` ata la medida al orden de busqueda.
int indiceDe(DallasTemperature& bus, int fallback) {
  (void)bus;
  return fallback;
}

// TODO(estudiante): offset en grados de este ROM (salida de calibrar_offset.py).
// T_corregida = T_leida + offset. No sumes nada a DEVICE_DISCONNECTED_C.
float offsetDe(const uint8_t* rom) {
  (void)rom;
  return 0.0f;
}

float leer(DallasTemperature& bus, int fallback) {
  const int index = indiceDe(bus, fallback);
  DeviceAddress addr;
  if (!bus.getAddress(addr, index)) return NAN;
  const float crudo = bus.getTempC(addr);
  if (crudo == DEVICE_DISCONNECTED_C) return NAN;
  return crudo + offsetDe(addr);
}

void convertir() {
  bool pending = false;
  if (ctrlBus.getDeviceCount() > 0) {
    ctrlBus.requestTemperatures();
    pending = true;
  }
  if (arrayBus.getDeviceCount() > 0) {
    arrayBus.requestTemperatures();
    pending = true;
  }
  if (pending) delay(CONVERSION_MS);
}

void logBus(const char* name, DallasTemperature& bus) {
  DeviceAddress addr;
  const int n = bus.getDeviceCount();
  Serial.printf("%s: %d\n", name, n);
  for (int i = 0; i < n; ++i) {
    if (!bus.getAddress(addr, i)) continue;
    Serial.printf("  [%d] ", i);
    printRom(addr);
    if (addr[0] != 0x28) Serial.print("  (no es DS18B20)");
    const float t = bus.getTempC(addr);
    if (t == DEVICE_DISCONNECTED_C) Serial.println("  sin lectura");
    else Serial.printf("  %.2f C\n", t);
  }
}

bool conectarWifi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.begin(LAB_WIFI_SSID, LAB_WIFI_PASSWORD);
  const uint32_t inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - inicio > WIFI_TIMEOUT_MS) return false;
    delay(250);
  }
  return true;
}

void reconnect() {
  while (!mqtt.connected()) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (mqtt.connect(client_id)) return;
    delay(2000);
  }
}

void publishTemps(float ctrl, const float* array_c) {
  JsonDocument doc;
  doc["ctrl"] = ctrl;
  JsonArray array = doc["array"].to<JsonArray>();
  for (int i = 0; i < ARRAY_COUNT; ++i) array.add(array_c[i]);
  doc["ts"] = millis() / 1000;
  char buf[256];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(topic_temps, buf);
  Serial.println(buf);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  ctrlBus.begin();
  arrayBus.begin();
  ctrlBus.setWaitForConversion(false);
  arrayBus.setWaitForConversion(false);
  ctrlBus.setResolution(12);
  arrayBus.setResolution(12);

  snprintf(topic_temps, sizeof(topic_temps), "lab/51535/%s/temps", LAB_GROUP_ID);
  snprintf(client_id, sizeof(client_id), "lab51535-%s", LAB_GROUP_ID);
  mqtt.setServer(LAB_MQTT_HOST, LAB_MQTT_PORT);
  mqtt.setBufferSize(512);
  Serial.println("Practica 3 — buses 1-Wire");
  logBus("ctrl", ctrlBus);
  logBus("array", arrayBus);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectarWifi();
  if (!mqtt.connected()) reconnect();
  mqtt.loop();

  convertir();
  logBus("ctrl", ctrlBus);
  logBus("array", arrayBus);

  const float ctrl = leer(ctrlBus, 0);
  float array_c[ARRAY_COUNT];
  for (int i = 0; i < ARRAY_COUNT; ++i) array_c[i] = leer(arrayBus, i);

  if (mqtt.connected()) publishTemps(ctrl, array_c);
  delay(PERIODO_MS);
}
