# 03 — Per-module Electrical Design

Full detail in [`PROJECT_SEED.md` §4](../PROJECT_SEED.md). Summary:

## Power rails
- **24 V (UHP-200-24):** heater only, up to 8.4 A. Inline 10 A fuse + 85 °C thermal fuse on the heater leg.
- **12 V (TXN 35-112):** ESP32 + 2 fans (~0.56 A). Comfortable headroom.
- Rails **share GND** (star-tied per module) so the MOSFET gate references correctly.

## Heater drive — two candidate paths (decide on the bench)
- **Path A (simple):** ESP32 GPIO → 100 Ω → gate of IRLB8721, 10 kΩ pulldown, direct 3.3 V drive.
- **Path B (robust):** ESP32 → MCP1407 gate driver (12 V) → IRFZ44N. Cooler MOSFET, one extra IC.
- **Rule:** ship Path A if MOSFET case stays < 70 °C at 8 A continuous with a clip-on heatsink; else Path B.

## Fan drive
MOSFET (IRLB8721, flyback diode per fan), abstracted in firmware so the
TTL-RELAY04 board is a drop-in fallback.

## Sensors
- 1× DFR0198 on the heater control bus (GPIO 4).
- 5× bare DS18B20 vertical array on a separate bus (GPIO 5), 4.7 kΩ pullup, ~10 cm spacing.

ESP32 pin map: see [`PROJECT_SEED.md` §4.5](../PROJECT_SEED.md) and the
authoritative copy in
[`firmware/esp32-module/include/board_config.h`](../firmware/esp32-module/include/board_config.h).

Schematics / wiring diagrams: [`wiring-diagrams/`](wiring-diagrams/).
