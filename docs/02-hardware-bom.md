# 02 — Hardware & Bill of Materials

See [`PROJECT_SEED.md` §3](../PROJECT_SEED.md) for the live BOM (received items
and items still to be sourced).

Exported machine-readable BOM CSVs go in [`../hardware/bom/`](../hardware/bom/).
Datasheets go in [`datasheets/`](datasheets/).

Key received hardware (per the April 2026 RS order):

- Heater PSU: Meanwell UHP-200-24 ×6
- Aux PSU: TRACOPOWER TXN 35-112 ×6
- MCU: ESP32-DEV-38P ×3 and ESP32-DEV-30P ×3
- Heater MOSFETs: IRLB8721 (Path A) and IRFZ44N + MCP1407 (Path B)
- Fans: RND 460-00033 120 mm 12 V ×24
- Control sensor: DFRobot DFR0198 (waterproof DS18B20) ×6
- Central controller: Raspberry Pi 5 4 GB ×2

Still to source: bare DS18B20 ×25+, NiChrome 80 wire, **85 °C thermal fuses
(safety-critical)**, input fuses, TO-220 heatsinks, passives, flyback diodes,
connectors, enclosures, aluminum mounting plate.
