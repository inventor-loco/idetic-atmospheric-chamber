# 01 — Overview

A 5-module atmospheric turbulence chamber for Free-Space Optical (FSO), Visible
Light Communication (VLC), and Optical Camera Communication (OCC) experiments at
IDeTIC / ULPGC.

- **Geometry:** 5 modules × 1 m = 5 m optical path, 50 × 50 cm cross-section, PMMA walls.
- **Turbulence generation:** per-module PWM-driven NiChrome heater wire → thermal convection.
- **Characterization:** per-module vertical array of 5 DS18B20 → Cn² from D_T(r).
- **Forced mixing:** two 120 mm fans per module for purging between experiments.
- **Orchestration:** each module = ESP32 over MQTT/WiFi to a central Raspberry Pi 5.

The authoritative, always-current description lives in [`PROJECT_SEED.md`](../PROJECT_SEED.md).
This `docs/` tree expands individual topics; when they disagree, the seed wins
until a doc is promoted to authoritative.
