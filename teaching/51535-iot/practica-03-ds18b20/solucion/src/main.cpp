// Practica 3 — solucion de referencia.
// Lee por ROM cuando la tabla de lab_config.h no esta a cero, aplica el
// offset y publica el objeto temps de docs/07. Un ROM ausente sale null.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <cstdio>

#include "lab_config.h"

constexpr int PIN_BUS_CTRL = 4;  // pins::CTRL_SENSOR_1WIRE. PIN_CTRL es un registro del SoC.
constexpr int PIN_ARRAY = 5;
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

bool romConfigured(const uint8_t rom[8]) {
  for (int i = 0; i < 8; ++i) {
    if (rom[i] != 0) return true;
  }
  return false;
}

bool romEqual(const uint8_t* a, const uint8_t* b) {
  for (int i = 0; i < 8; ++i) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void printRom(const uint8_t* rom) {
  for (int i = 0; i < 8; ++i) {
    if (i) Serial.print(':');
    if (rom[i] < 16) Serial.print('0');
    Serial.print(rom[i], HEX);
  }
}

int indicePorRom(DallasTemperature& bus, const uint8_t rom[8]) {
  DeviceAddress addr;
  const int n = bus.getDeviceCount();
  for (int i = 0; i < n; ++i) {
    if (bus.getAddress(addr, i) && romEqual(addr, rom)) return i;
  }
  return -1;
}

float leer(DallasTemperature& bus, const uint8_t rom[8], int fallback, float offset) {
  int index = fallback;
  if (romConfigured(rom)) {
    index = indicePorRom(bus, rom);
    if (index < 0) return NAN;
  } else if (fallback >= bus.getDeviceCount()) {
    return NAN;
  }
  DeviceAddress addr;
  if (!bus.getAddress(addr, index)) return NAN;
  const float crudo = bus.getTempC(addr);
  if (crudo == DEVICE_DISCONNECTED_C) return NAN;
  return crudo + offset;
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

  Serial.println("Practica 3 — solucion");
  if (!romConfigured(LAB_ROM_CTRL)) {
    Serial.println("ROM de control a cero: se usa el indice 0 y offset 0.");
  }
  logBus("ctrl", ctrlBus);
  logBus("array", arrayBus);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectarWifi();
  if (!mqtt.connected()) reconnect();
  mqtt.loop();

  convertir();
  const float ctrl = leer(ctrlBus, LAB_ROM_CTRL, 0, LAB_OFFSET_CTRL);
  float array_c[ARRAY_COUNT];
  for (int i = 0; i < ARRAY_COUNT; ++i) {
    array_c[i] = leer(arrayBus, LAB_ROM_ARRAY[i], i, LAB_OFFSET_ARRAY[i]);
  }
  if (mqtt.connected()) publishTemps(ctrl, array_c);
  delay(PERIODO_MS);
}
