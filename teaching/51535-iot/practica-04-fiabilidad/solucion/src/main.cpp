// Practica 4 — solucion de referencia.
// Reconexion periodica, SAFE_OFF enclavado a los 30 s de enlace caido
// si el actuador estaba en RUN, fault al recuperar el broker.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <cstdio>
#include <cstring>

#include "lab_config.h"

constexpr int PIN_LED = 2;
constexpr int PIN_SENSOR = 34;
constexpr uint32_t PERIODO_MS = 1000;
constexpr uint32_t RECONNECT_MS = 2000;
constexpr uint32_t WIFI_TIMEOUT_MS = 15000;

enum class Estado { IDLE, RUN, SAFE_OFF };

WiFiClient net;
PubSubClient mqtt(net);

char topic_hb[64];
char topic_status[64];
char topic_fault[64];
char topic_stop[64];
char topic_run[64];
char topic_all_stop[64];
char client_id[32];

Estado estado = Estado::IDLE;
uint32_t seq = 0;
uint32_t last_tick = 0;
uint32_t last_mqtt_try = 0;
uint32_t last_wifi_try = 0;
uint32_t down_since = 0;
bool ever_connected = false;
char fault_code[24] = "";
float fault_value = 0.0f;
bool fault_pending = false;

const char* nombreEstado() {
  switch (estado) {
    case Estado::RUN: return "RUN";
    case Estado::SAFE_OFF: return "SAFE_OFF";
    case Estado::IDLE: return "IDLE";
  }
  return "IDLE";
}

void enterSafeOff(const char* code, float value) {
  digitalWrite(PIN_LED, LOW);
  if (estado == Estado::SAFE_OFF) return;
  estado = Estado::SAFE_OFF;
  snprintf(fault_code, sizeof(fault_code), "%s", code);
  fault_value = value;
  fault_pending = true;
  Serial.printf("SAFE_OFF %s\n", fault_code);
}

void onMqtt(char* topic, uint8_t* payload, unsigned int len) {
  (void)payload;
  (void)len;
  if (strcmp(topic, topic_stop) == 0 || strcmp(topic, topic_all_stop) == 0) {
    enterSafeOff("STOP_CMD", 0.0f);
    return;
  }
  if (strcmp(topic, topic_run) == 0 && mqtt.connected()) {
    estado = Estado::RUN;
    digitalWrite(PIN_LED, HIGH);
    Serial.println("RUN");
  }
}

void suscribir() {
  mqtt.subscribe(topic_stop, 1);
  mqtt.subscribe(topic_run, 1);
  mqtt.subscribe(topic_all_stop, 1);
}

bool esperarWifi(uint32_t timeout_ms) {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.begin(LAB_WIFI_SSID, LAB_WIFI_PASSWORD);
  const uint32_t inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - inicio > timeout_ms) return false;
    delay(250);
  }
  Serial.printf("WiFi %s\n", WiFi.localIP().toString().c_str());
  return true;
}

void maintainLink() {
  const uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (last_wifi_try == 0 || now - last_wifi_try >= 5000) {
      last_wifi_try = now;
      WiFi.mode(WIFI_STA);
      WiFi.begin(LAB_WIFI_SSID, LAB_WIFI_PASSWORD);
    }
  } else if (!mqtt.connected()) {
    if (last_mqtt_try == 0 || now - last_mqtt_try >= RECONNECT_MS) {
      last_mqtt_try = now;
      if (mqtt.connect(client_id)) {
        suscribir();
        Serial.println("MQTT ok");
      }
    }
  }

  if (mqtt.connected()) {
    ever_connected = true;
    down_since = 0;
    return;
  }
  if (down_since == 0) down_since = now == 0 ? 1 : now;
  if (ever_connected && estado == Estado::RUN &&
      millis() - down_since > LAB_MQTT_TIMEOUT_MS) {
    enterSafeOff("MQTT_TIMEOUT", static_cast<float>(LAB_MQTT_TIMEOUT_MS) / 1000.0f);
  }
}

void publishHeartbeat(uint32_t n) {
  JsonDocument doc;
  doc["seq"] = n;
  doc["adc"] = analogRead(PIN_SENSOR);
  doc["ts"] = millis() / 1000;
  doc["rssi"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  doc["state"] = nombreEstado();
  char buf[192];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(topic_hb, buf);
}

void publishStatus() {
  JsonDocument doc;
  doc["state"] = nombreEstado();
  doc["pwm"] = (estado == Estado::RUN) ? 1.0 : 0.0;
  doc["fan"] = false;
  doc["uptime_s"] = millis() / 1000;
  doc["rssi"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  char buf[192];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(topic_status, buf, /*retain=*/true);
}

void flushFault() {
  if (!fault_pending || !mqtt.connected()) return;
  JsonDocument doc;
  doc["code"] = fault_code;
  doc["value"] = fault_value;
  doc["ts"] = millis() / 1000;
  char buf[160];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(topic_fault, buf);
  fault_pending = false;
  Serial.printf("fault %s\n", buf);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  analogReadResolution(12);

  snprintf(topic_hb, sizeof(topic_hb), "lab/51535/%s/heartbeat", LAB_GROUP_ID);
  snprintf(topic_status, sizeof(topic_status), "lab/51535/%s/status", LAB_GROUP_ID);
  snprintf(topic_fault, sizeof(topic_fault), "lab/51535/%s/fault", LAB_GROUP_ID);
  snprintf(topic_stop, sizeof(topic_stop), "lab/51535/%s/cmd/stop", LAB_GROUP_ID);
  snprintf(topic_run, sizeof(topic_run), "lab/51535/%s/cmd/run", LAB_GROUP_ID);
  snprintf(topic_all_stop, sizeof(topic_all_stop), "lab/51535/all/cmd/stop");
  snprintf(client_id, sizeof(client_id), "lab51535-%s", LAB_GROUP_ID);

  mqtt.setServer(LAB_MQTT_HOST, LAB_MQTT_PORT);
  mqtt.setCallback(onMqtt);
  mqtt.setBufferSize(512);
  mqtt.setSocketTimeout(2);
  esperarWifi(WIFI_TIMEOUT_MS);
  Serial.println("Practica 4 — solucion. LED apagado hasta cmd/run.");
}

void loop() {
  const uint32_t now = millis();
  if (last_tick == 0) last_tick = now;
  while (now - last_tick >= PERIODO_MS) {
    last_tick += PERIODO_MS;
    const uint32_t n = seq++;
    if (mqtt.connected()) {
      publishHeartbeat(n);
      publishStatus();
    }
  }
  maintainLink();
  mqtt.loop();
  flushFault();
}
