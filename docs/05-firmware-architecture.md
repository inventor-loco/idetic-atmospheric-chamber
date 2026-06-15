# 05 — Firmware Architecture (ESP32)

Detail in [`PROJECT_SEED.md` §5](../PROJECT_SEED.md). Code:
[`firmware/esp32-module/`](../firmware/esp32-module/).

## Stack
PlatformIO + Arduino framework. Libraries: `OneWire` + `DallasTemperature`,
`PubSubClient`, `ArduinoJson`, built-in `LEDC` + WiFi.

## Module layout
| File | Responsibility |
|---|---|
| `main.cpp` | 1 Hz control loop, state transitions, telemetry |
| `state_machine.{h,cpp}` | `State` enum + names (IDLE…FAULT) |
| `heater.{h,cpp}` | LEDC PWM + PID; `cutoff()` always safe |
| `sensors.{h,cpp}` | DS18B20 control + 5-element array reads |
| `fan.{h,cpp}` | on/off, hardware-abstracted (MOSFET vs relay) |
| `safety.{h,cpp}` | every-cycle kill conditions, MQTT-independent |
| `mqtt_client.{h,cpp}` | WiFi/MQTT, topic schema, command decode |
| `board_config.h` | pin map, PWM/safety/timing constants |

## State machine
`IDLE → HEATING → STEADY_STATE`, with `PURGING → SETTLING → READY`, plus
`SAFE_OFF` (recoverable) and `FAULT` (latched, manual reset). See seed §5.

## Control & telemetry
- PID on the control sensor → PWM duty. 1 Hz sample (DS18B20 conversion ~750 ms).
- Telemetry 1 Hz: 6 temps, PWM, fan, fault flags. 0.1 Hz: RSSI, uptime, heap.

## Tests
Pure-logic units (PID, `evaluateSafety`, command decode) → native host tests;
1-Wire/LEDC → on-target. See [`../firmware/esp32-module/test/`](../firmware/esp32-module/test/).
