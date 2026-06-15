// safety.h — firmware safety invariants (PROJECT_SEED §5, §8).
//
// These checks run every control cycle, independent of MQTT state. They are
// the software layer ON TOP of hardware protection (thermal fuse + gate
// pulldown), never a replacement for it.
#pragma once
#include "sensors.h"

enum class FaultCode {
  NONE,
  OVERTEMP_CTRL,    // control sensor > CTRL_OVERTEMP_C
  OVERTEMP_ARRAY,   // any array sensor > ARRAY_OVERTEMP_C (PMMA protection)
  SENSOR_FAIL,      // too many consecutive 1-Wire failures
  MQTT_TIMEOUT      // broker unreachable > MQTT_TIMEOUT_MS
};

const char* faultName(FaultCode c);

// Evaluates all kill conditions. Returns the first tripped fault, or NONE.
// `mqtt_age_ms` is the time since the last message from the broker.
FaultCode evaluateSafety(const SensorReadings& r, int consecutive_failures,
                         uint32_t mqtt_age_ms);
