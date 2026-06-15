// heater.h — NiChrome heater PWM drive + PID control (PROJECT_SEED §4.2, §5).
#pragma once
#include <stdint.h>

struct PidGains {
  float kp = 5.0f;
  float ki = 0.1f;
  float kd = 0.0f;
};

class Heater {
 public:
  // Configures LEDC and forces duty to 0. Call once in setup().
  // Hardware gate pulldown guarantees OFF before this runs (PROJECT_SEED §8).
  void begin();

  // Open-loop: set duty 0.0..1.0 directly (bench testing / manual mode).
  void setDuty(float duty01);

  // Closed-loop: set target control-sensor temperature in °C.
  void setSetpoint(float target_c);

  // Run one PID step against the latest control-sensor reading.
  // Returns the duty applied (0.0..1.0). Call at CONTROL_PERIOD_MS cadence.
  float update(float ctrl_temp_c, float dt_s);

  // Immediately force duty to 0 (safety cutoff). Always safe to call.
  void cutoff();

  void   setGains(const PidGains& g) { gains_ = g; }
  float  duty() const { return duty_; }

 private:
  void   applyDuty_(float duty01);

  PidGains gains_;
  float setpoint_c_ = 0.0f;
  float duty_       = 0.0f;
  float integral_   = 0.0f;
  float prev_err_   = 0.0f;
  bool  closed_loop_ = false;
};
