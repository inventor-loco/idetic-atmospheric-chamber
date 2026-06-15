#include "heater.h"
#include "board_config.h"
#include <Arduino.h>

void Heater::begin() {
  ledcSetup(HEATER_LEDC_CHANNEL, HEATER_PWM_FREQ_HZ, HEATER_PWM_RES_BITS);
  ledcAttachPin(pins::HEATER_PWM, HEATER_LEDC_CHANNEL);
  cutoff();
}

void Heater::applyDuty_(float duty01) {
  if (duty01 < 0.0f) duty01 = 0.0f;
  if (duty01 > 1.0f) duty01 = 1.0f;
  duty_ = duty01;
  const uint32_t max_duty = (1u << HEATER_PWM_RES_BITS) - 1u;
  ledcWrite(HEATER_LEDC_CHANNEL, static_cast<uint32_t>(duty01 * max_duty));
}

void Heater::setDuty(float duty01) {
  closed_loop_ = false;
  applyDuty_(duty01);
}

void Heater::setSetpoint(float target_c) {
  closed_loop_ = true;
  setpoint_c_  = target_c;
  integral_    = 0.0f;
  prev_err_    = 0.0f;
}

float Heater::update(float ctrl_temp_c, float dt_s) {
  if (!closed_loop_ || dt_s <= 0.0f) return duty_;

  const float err = setpoint_c_ - ctrl_temp_c;
  integral_ += err * dt_s;
  // Clamp integral to the actuator range to avoid wind-up.
  if (integral_ > 1.0f / (gains_.ki + 1e-6f)) integral_ = 1.0f / (gains_.ki + 1e-6f);
  if (integral_ < 0.0f) integral_ = 0.0f;

  const float deriv = (err - prev_err_) / dt_s;
  prev_err_ = err;

  const float out = gains_.kp * err + gains_.ki * integral_ + gains_.kd * deriv;
  applyDuty_(out);  // applyDuty_ clamps to [0,1]
  return duty_;
}

void Heater::cutoff() {
  closed_loop_ = false;
  applyDuty_(0.0f);
  integral_ = 0.0f;
  prev_err_ = 0.0f;
}
