// sensors.h — DS18B20 reads: 1 control sensor + 5-element Cn² array.
// PROJECT_SEED §4.4. Two separate 1-Wire buses on different GPIOs.
#pragma once
#include <stdint.h>
#include "board_config.h"

struct SensorReadings {
  float ctrl_c = NAN;                         // DFR0198 heater control sensor
  float array_c[ARRAY_SENSOR_COUNT] = {NAN};  // vertical Cn² array, bottom→top
  bool  ctrl_ok = false;
  bool  array_ok = false;                     // true only if all array reads ok
};

class Sensors {
 public:
  void begin();

  // Triggers a conversion and reads both buses. DS18B20 12-bit conversion
  // takes ~750 ms, so call no faster than 1 Hz (CONTROL_PERIOD_MS).
  SensorReadings read();

  // Consecutive failed read cycles, used for the SENSOR_FAIL_LIMIT cutoff.
  int consecutiveFailures() const { return fail_count_; }

 private:
  int fail_count_ = 0;
};
