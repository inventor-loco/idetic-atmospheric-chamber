// board_config.h — pin map and per-board constants for the chamber module ESP32.
//
// Pin assignments follow PROJECT_SEED §4.5. The heater PWM pin is deliberately
// chosen to exist on BOTH the 38-pin and 30-pin dev boards so a single mapping
// works for either variant (PROJECT_SEED §11). Define BOARD_ESP32_DEV_38P or
// BOARD_ESP32_DEV_30P via build flags in platformio.ini.

#pragma once

#if !defined(BOARD_ESP32_DEV_38P) && !defined(BOARD_ESP32_DEV_30P)
#error "Define BOARD_ESP32_DEV_38P or BOARD_ESP32_DEV_30P (set in platformio.ini)"
#endif

// ---------------------------------------------------------------------------
// GPIO assignments (identical across both board variants for now)
// ---------------------------------------------------------------------------
namespace pins {
constexpr int HEATER_PWM        = 25;  // LEDC ch 0, MOSFET gate (via 100R)
constexpr int CTRL_SENSOR_1WIRE = 4;   // DFR0198 heater control sensor
constexpr int ARRAY_1WIRE       = 5;   // 5x DS18B20 Cn2 array (4.7k pullup)
constexpr int FAN_SWITCH        = 26;  // MOSFET gate for both fans
constexpr int STATUS_LED        = 2;   // onboard blue LED
constexpr int THERMAL_FUSE_STAT = 34;  // input-only, monitors 85C fuse leg
constexpr int CURRENT_SENSE_ADC = 35;  // reserved, future current sense
}  // namespace pins

// ---------------------------------------------------------------------------
// Heater PWM (LEDC) configuration — PROJECT_SEED §4.5
// ---------------------------------------------------------------------------
constexpr int   HEATER_LEDC_CHANNEL = 0;
constexpr int   HEATER_PWM_FREQ_HZ  = 200;   // within 100–500 Hz band
constexpr int   HEATER_PWM_RES_BITS = 10;    // 0..1023 duty resolution

// ---------------------------------------------------------------------------
// Safety limits — PROJECT_SEED §5/§8. Firmware-enforced, independent of MQTT.
// ---------------------------------------------------------------------------
constexpr float CTRL_OVERTEMP_C       = 90.0f;  // control sensor hard cutoff
constexpr float ARRAY_OVERTEMP_C      = 60.0f;  // PMMA wall protection
constexpr uint32_t MQTT_TIMEOUT_MS    = 30000;  // -> SAFE_OFF if exceeded
constexpr int   SENSOR_FAIL_LIMIT     = 5;      // consecutive 1-Wire errors

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
constexpr uint32_t CONTROL_PERIOD_MS  = 1000;   // 1 Hz (DS18B20 conversion)
constexpr int   ARRAY_SENSOR_COUNT    = 5;
