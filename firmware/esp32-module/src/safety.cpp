#include "safety.h"
#include "board_config.h"

const char* faultName(FaultCode c) {
  switch (c) {
    case FaultCode::NONE:           return "NONE";
    case FaultCode::OVERTEMP_CTRL:  return "OVERTEMP_CTRL";
    case FaultCode::OVERTEMP_ARRAY: return "OVERTEMP_ARRAY";
    case FaultCode::SENSOR_FAIL:    return "SENSOR_FAIL";
    case FaultCode::MQTT_TIMEOUT:   return "MQTT_TIMEOUT";
  }
  return "UNKNOWN";
}

FaultCode evaluateSafety(const SensorReadings& r, int consecutive_failures,
                         uint32_t mqtt_age_ms) {
  // Sensor failure first: if we can't trust readings, we can't trust the rest.
  if (consecutive_failures >= SENSOR_FAIL_LIMIT) return FaultCode::SENSOR_FAIL;

  if (r.ctrl_ok && r.ctrl_c > CTRL_OVERTEMP_C) return FaultCode::OVERTEMP_CTRL;

  if (r.array_ok) {
    for (int i = 0; i < ARRAY_SENSOR_COUNT; ++i) {
      if (r.array_c[i] > ARRAY_OVERTEMP_C) return FaultCode::OVERTEMP_ARRAY;
    }
  }

  if (mqtt_age_ms > MQTT_TIMEOUT_MS) return FaultCode::MQTT_TIMEOUT;

  return FaultCode::NONE;
}
