# 04 — Safety (non-negotiable)

These constraints apply during prototype bench testing **and** final operation.
Authoritative list: [`PROJECT_SEED.md` §8](../PROJECT_SEED.md).

1. **Hardware over-temperature:** 85 °C axial thermal fuse in series with each heater, bonded to the worst-case PMMA hotspot. Independent of firmware. **Do not power a heater without it.**
2. **MOSFET gate pulldown:** 10 kΩ G→S on every MOSFET. Heater is OFF at boot, during ESP32 reset, and during firmware update — never rely on firmware for boot-time safety.
3. **Per-module input fuse:** 10 A fast-blow on the 24 V rail, before any switching element.
4. **Firmware kill conditions** (see [`05-firmware-architecture.md`](05-firmware-architecture.md) and `safety.cpp`):
   - control sensor > 90 °C → heater 0 %
   - any array sensor > 60 °C → heater 0 % (PMMA wall protection)
   - MQTT lost > 30 s → SAFE_OFF
   - 1-Wire read fails 5 cycles → SAFE_OFF
5. **Mains safety:** earthed enclosure, strain relief on every AC entry, Class I install per the UHP-200-24 datasheet.
6. **NiChrome clearance:** ≥ 8 cm to any PMMA surface, suspended on ceramic beads, never in contact.
7. **Operation:** no unattended heating until all software checks have passed for ≥ 10 min; UI must show a clear **ARMED** indicator.
