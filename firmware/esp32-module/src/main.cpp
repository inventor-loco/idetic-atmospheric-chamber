// main.cpp — chamber module firmware entry point.
//
// Wires together sensors, heater PID, fan, the safety layer, and MQTT into a
// 1 Hz control loop with the state machine from PROJECT_SEED §5. This is a
// Phase-0/1 skeleton: the control loop, safety cutoffs, and telemetry are
// real; the purge/settle timing is stubbed where marked TODO.

#include <Arduino.h>
#include <WiFi.h>
#include "board_config.h"
#include "state_machine.h"
#include "sensors.h"
#include "heater.h"
#include "fan.h"
#include "safety.h"
#include "mqtt_client.h"
#include "ota.h"

namespace {
Sensors    sensors;
Heater     heater;
Fan        fan;
MqttClient mqtt;
Ota        ota;

State    state = State::IDLE;
uint32_t last_control_ms = 0;
uint32_t purge_until_ms  = 0;
uint32_t settle_until_ms = 0;

// OTA is handled in the main loop (never inside the MQTT callback): the command
// only stages a request, which loop() runs once the heater is confirmed off.
bool     ota_pending = false;
Command  ota_req;

// OTA is permitted only in states where the heater is already off. It is
// refused while heating/purging so an update can never interrupt a live heater.
bool otaAllowed(State s) {
  return s == State::IDLE || s == State::SAFE_OFF ||
         s == State::READY || s == State::FAULT;
}

void enterSafeOff(const char* reason) {
  heater.cutoff();
  fan.set(true);             // fans on while cooling down
  state = State::SAFE_OFF;
  mqtt.publishFault(reason, 0.0f, millis() / 1000);
}

void onCommand(const Command& c) {
  switch (c.type) {
    case Command::SETPOINT:
      heater.setSetpoint(c.target_c);
      if (state == State::IDLE || state == State::SAFE_OFF || state == State::READY)
        state = State::HEATING;
      break;
    case Command::PWM:
      heater.setDuty(c.pwm_pct / 100.0f);
      state = State::HEATING;
      break;
    case Command::FAN:
      fan.set(c.fan_on);
      break;
    case Command::PURGE:
      heater.cutoff();
      fan.set(true);
      purge_until_ms  = millis() + c.purge_s * 1000;
      settle_until_ms = purge_until_ms + c.settle_s * 1000;
      state = State::PURGING;
      break;
    case Command::STOP:
      enterSafeOff("STOP_CMD");
      break;
    case Command::CONFIG:
      heater.setGains({c.kp, c.ki, c.kd});
      break;
    case Command::OTA:
      if (otaAllowed(state)) {
        heater.cutoff();          // belt-and-braces before staging the update
        ota_req = c;
        ota_pending = true;
      } else {
        mqtt.publishOta("REJECTED", 0, "unsafe_state", c.ota_version.c_str());
      }
      break;
    default:
      break;
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(pins::STATUS_LED, OUTPUT);
  pinMode(pins::THERMAL_FUSE_STAT, INPUT);

  sensors.begin();
  heater.begin();   // forces duty 0 (hardware pulldown guarantees boot-safe)
  fan.begin();
  mqtt.begin(onCommand);
  ota.begin([](const char* phase, int progress, const char* err) {
    mqtt.publishOta(phase, progress, err, ota_req.ota_version.c_str());
  });

  Serial.printf("chamber module %s fw %s booted\n", MODULE_ID, CHAMBER_FW_VERSION);
}

void loop() {
  mqtt.loop();

  // Staged OTA runs here, outside the MQTT callback. ota.run() blocks while it
  // streams the image and reboots on success; on failure we stay in SAFE_OFF on
  // the current firmware. Heater is already cut (onCommand + here).
  if (ota_pending) {
    ota_pending = false;
    heater.cutoff();
    state = State::SAFE_OFF;
    ota.run(ota_req.ota_url, ota_req.ota_md5, ota_req.ota_size);
    return;
  }

  const uint32_t now = millis();
  if (now - last_control_ms < CONTROL_PERIOD_MS) return;
  const float dt_s = (now - last_control_ms) / 1000.0f;
  last_control_ms = now;

  // 1) Read sensors.
  SensorReadings r = sensors.read();

  // 2) Safety layer — runs every cycle regardless of state (PROJECT_SEED §8).
  FaultCode fault = evaluateSafety(r, sensors.consecutiveFailures(),
                                   mqtt.msSinceLastBrokerMsg());
  if (fault != FaultCode::NONE && state != State::FAULT) {
    heater.cutoff();
    fan.set(true);
    // MQTT timeout is recoverable -> SAFE_OFF; the rest latch as FAULT.
    state = (fault == FaultCode::MQTT_TIMEOUT) ? State::SAFE_OFF : State::FAULT;
    mqtt.publishFault(faultName(fault),
                      fault == FaultCode::OVERTEMP_CTRL ? r.ctrl_c : 0.0f,
                      now / 1000);
  }

  // 3) State-driven actuation.
  switch (state) {
    case State::HEATING:
    case State::STEADY_STATE:
      if (r.ctrl_ok) heater.update(r.ctrl_c, dt_s);
      // TODO: transition HEATING -> STEADY_STATE on setpoint-reached hysteresis.
      break;
    case State::PURGING:
      if (now >= purge_until_ms) state = State::SETTLING;
      break;
    case State::SETTLING:
      fan.set(false);
      if (now >= settle_until_ms) state = State::READY;
      break;
    default:
      break;  // IDLE / READY / SAFE_OFF / FAULT: heater already off
  }

  // 4) Telemetry (PROJECT_SEED §7.1).
  mqtt.publishTemps(r, now / 1000);
  mqtt.publishStatus(stateName(state), heater.duty(), fan.isOn(),
                     now / 1000, WiFi.RSSI());

  digitalWrite(pins::STATUS_LED, mqtt.connected() ? HIGH : LOW);
}
