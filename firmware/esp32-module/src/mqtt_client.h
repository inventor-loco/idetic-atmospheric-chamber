// mqtt_client.h — WiFi + MQTT transport and chamber topic schema.
// PROJECT_SEED §5, §7. Wraps PubSubClient and handles (re)connection.
#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <functional>
#include "sensors.h"

// A decoded command from the Pi (PROJECT_SEED §7.2).
struct Command {
  enum Type { NONE, SETPOINT, PWM, FAN, PURGE, STOP, CONFIG, OTA } type = NONE;
  float    target_c   = 0.0f;
  float    pwm_pct    = 0.0f;
  bool     fan_on     = false;
  uint32_t purge_s    = 0;
  uint32_t settle_s   = 0;
  // CONFIG payload (PID + limits) parsed straight into these:
  float    kp = 0, ki = 0, kd = 0;
  // OTA payload (PROJECT_SEED §7.2): firmware image to pull + integrity check.
  String   ota_url;
  String   ota_md5;
  String   ota_version;
  size_t   ota_size = 0;
};

class MqttClient {
 public:
  using CommandHandler = std::function<void(const Command&)>;

  void begin(CommandHandler on_command);
  void loop();                       // call every iteration; handles reconnect
  bool connected() const;
  uint32_t msSinceLastBrokerMsg() const;

  // Publishers (PROJECT_SEED §7.1).
  void publishTemps(const SensorReadings& r, uint32_t ts);
  void publishStatus(const char* state, float pwm, bool fan,
                     uint32_t uptime_s, int rssi);
  void publishFault(const char* code, float value, uint32_t ts);
  // OTA progress/result (PROJECT_SEED §7.1). phase: STARTED/DOWNLOADING/
  // SUCCESS/FAILED/REJECTED. Published QoS 1, not retained.
  void publishOta(const char* phase, int progress, const char* err,
                  const char* version);

 private:
  void reconnect_();
  void subscribe_();

  CommandHandler on_command_;
  uint32_t last_broker_msg_ms_ = 0;
};
