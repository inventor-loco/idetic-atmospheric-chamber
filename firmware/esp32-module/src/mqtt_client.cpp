#include "mqtt_client.h"
#include "board_config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

namespace {
WiFiClient   net;
PubSubClient mqtt(net);

// Topic helpers. MODULE_ID is injected from secrets.ini (e.g. "m1").
String base() { return String("chamber/") + MODULE_ID; }

MqttClient* g_self = nullptr;  // for the C-style PubSubClient callback

// Parses an inbound command topic + JSON payload into a Command.
Command decode(const String& topic, const JsonDocument& doc) {
  Command c;
  if (topic.endsWith("/cmd/setpoint")) {
    if (doc["target_c"].is<float>()) { c.type = Command::SETPOINT; c.target_c = doc["target_c"]; }
    else if (doc["pwm_pct"].is<float>()) { c.type = Command::PWM; c.pwm_pct = doc["pwm_pct"]; }
  } else if (topic.endsWith("/cmd/fan")) {
    c.type = Command::FAN; c.fan_on = doc["on"] | false;
  } else if (topic.endsWith("/cmd/purge")) {
    c.type = Command::PURGE; c.purge_s = doc["duration_s"] | 60; c.settle_s = doc["settle_s"] | 30;
  } else if (topic.endsWith("/cmd/stop")) {
    c.type = Command::STOP;
  } else if (topic.endsWith("/cmd/config")) {
    c.type = Command::CONFIG;
    c.kp = doc["pid"]["kp"] | 0.0f;
    c.ki = doc["pid"]["ki"] | 0.0f;
    c.kd = doc["pid"]["kd"] | 0.0f;
  } else if (topic.endsWith("/cmd/ota")) {
    c.type = Command::OTA;
    c.ota_url     = String(doc["url"]     | "");
    c.ota_md5     = String(doc["md5"]     | "");
    c.ota_version = String(doc["version"] | "");
    c.ota_size    = doc["size"] | 0;
  }
  return c;
}
}  // namespace

void MqttClient::begin(CommandHandler on_command) {
  g_self = this;
  on_command_ = std::move(on_command);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback([](char* topic, uint8_t* payload, unsigned int len) {
    if (!g_self) return;
    g_self->last_broker_msg_ms_ = millis();
    JsonDocument doc;
    if (deserializeJson(doc, payload, len)) return;  // ignore malformed
    Command c = decode(String(topic), doc);
    if (c.type != Command::NONE && g_self->on_command_) g_self->on_command_(c);
  });
}

void MqttClient::reconnect_() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connect(MODULE_ID)) {
    last_broker_msg_ms_ = millis();
    subscribe_();
  }
}

void MqttClient::subscribe_() {
  // Per-module and broadcast command topics (PROJECT_SEED §7.2, §7.3).
  String t = base() + "/cmd/+";
  mqtt.subscribe(t.c_str(), /*qos=*/1);
  mqtt.subscribe("chamber/all/cmd/+", /*qos=*/1);
}

void MqttClient::loop() {
  if (!mqtt.connected()) {
    static uint32_t last_try = 0;
    if (millis() - last_try > 2000) { last_try = millis(); reconnect_(); }
  }
  mqtt.loop();
}

bool MqttClient::connected() const { return mqtt.connected(); }

uint32_t MqttClient::msSinceLastBrokerMsg() const {
  return millis() - last_broker_msg_ms_;
}

void MqttClient::publishTemps(const SensorReadings& r, uint32_t ts) {
  JsonDocument doc;
  doc["ctrl"] = r.ctrl_c;
  JsonArray a = doc["array"].to<JsonArray>();
  for (int i = 0; i < ARRAY_SENSOR_COUNT; ++i) a.add(r.array_c[i]);
  doc["ts"] = ts;
  char buf[256];
  size_t n = serializeJson(doc, buf);
  String t = base() + "/temps";
  mqtt.publish(t.c_str(), reinterpret_cast<uint8_t*>(buf), n, /*retain=*/false);
}

void MqttClient::publishStatus(const char* state, float pwm, bool fan,
                               uint32_t uptime_s, int rssi) {
  JsonDocument doc;
  doc["state"] = state;
  doc["pwm"] = pwm;
  doc["fan"] = fan;
  doc["uptime_s"] = uptime_s;
  doc["rssi"] = rssi;
  doc["fw"] = CHAMBER_FW_VERSION;   // lets the orchestrator confirm OTA result
  char buf[192];
  size_t n = serializeJson(doc, buf);
  String t = base() + "/status";
  mqtt.publish(t.c_str(), reinterpret_cast<uint8_t*>(buf), n, /*retain=*/true);
}

void MqttClient::publishFault(const char* code, float value, uint32_t ts) {
  JsonDocument doc;
  doc["code"] = code;
  doc["value"] = value;
  doc["ts"] = ts;
  char buf[128];
  size_t n = serializeJson(doc, buf);
  String t = base() + "/fault";
  mqtt.publish(t.c_str(), reinterpret_cast<uint8_t*>(buf), n, /*retain=*/false);
}

void MqttClient::publishOta(const char* phase, int progress, const char* err,
                            const char* version) {
  JsonDocument doc;
  doc["phase"] = phase;
  doc["progress"] = progress;
  doc["version"] = version;
  if (err && err[0]) doc["error"] = err;
  char buf[192];
  size_t n = serializeJson(doc, buf);
  String t = base() + "/ota";
  mqtt.publish(t.c_str(), reinterpret_cast<uint8_t*>(buf), n, /*retain=*/false);
  // Push it out immediately — a reboot may follow right after SUCCESS.
  mqtt.loop();
}
