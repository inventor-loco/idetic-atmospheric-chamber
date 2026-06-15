# Atmospheric Turbulence Chamber — Project Seed

> **Purpose of this document.** This is the entry-point context for Claude Code (and any new collaborator). It captures the locked-in architectural decisions, the current hardware state, and the immediate roadmap. Read this first before touching any subdirectory.

---

## 1. What we are building

A **5-module atmospheric turbulence chamber** for Free-Space Optical (FSO), Visible Light Communication (VLC), and Optical Camera Communication (OCC) experiments at IDeTIC / ULPGC.

- **Geometry.** 5 modules × 1 m each, total 5 m optical path. Cross-section 50 × 50 cm. Walls in PMMA.
- **Turbulence generation.** Each module has a hot NiChrome wire serpentined across the bottom on ceramic standoffs, PWM-driven via MOSFET, creating thermal convection and refractive-index fluctuations.
- **Turbulence characterization.** Each module has a vertical array of 5 DS18B20 sensors on the side wall to estimate Cn² from the temperature structure function D_T(r).
- **Forced mixing.** Two 120 mm 12 V fans per module (top + opposite side) for purging the chamber between experiments. On/off only, no PWM speed control.
- **Orchestration.** Each module runs an ESP32 talking MQTT over WiFi to a central Raspberry Pi 5 with touchscreen. The Pi runs the broker, the control loops, the UI, and data logging.

**Scientific targets.**
- Heater surface temperature: 300–400 °C (hot but not glowing).
- Air temperature gradient: ΔT 30–60 K between heater zone and upper sensor.
- PMMA inner wall: stay below 60 °C (HDT ≈ 95 °C, with margin).
- Continuous operation for multi-hour experiments without supervision.

---

## 2. System architecture (one paragraph)

Each chamber **module** is electrically self-contained: a Meanwell UHP-200-24 PSU feeds the NiChrome heater through a logic-level MOSFET (PWM at 100–500 Hz), a Traco TXN 35-112 feeds the ESP32 and both fans through a second switch. The ESP32 reads 1 closed-loop temperature sensor on the wire support and 5 sensors on the vertical array, runs a local PID, and publishes telemetry over WiFi MQTT to a central **Raspberry Pi 5** that hosts the Mosquitto broker, a Python orchestrator (per-module state machines, purge cycles, data logging), and a touchscreen UI for live setpoint control and Cn² readout.

```
                      ┌──────────────────────────────────────────┐
                      │  Raspberry Pi 5 + 7" touchscreen          │
                      │  - Mosquitto broker                       │
                      │  - Python orchestrator (asyncio)          │
                      │  - Touch UI (Qt or web)                   │
                      │  - HDF5 / InfluxDB / CSV logging          │
                      └─────────────────┬────────────────────────┘
                                        │  WiFi (MQTT)
              ┌─────────────────┬───────┴──────────┬─────────────────┬─────────────────┐
              │                 │                  │                 │                 │
        ┌─────▼─────┐     ┌─────▼─────┐      ┌─────▼─────┐    ┌─────▼─────┐    ┌─────▼─────┐
        │ Module 1  │     │ Module 2  │      │ Module 3  │    │ Module 4  │    │ Module 5  │
        │  ESP32    │     │  ESP32    │      │  ESP32    │    │  ESP32    │    │  ESP32    │
        │  + heater │     │  + heater │      │  + heater │    │  + heater │    │  + heater │
        │  + fans   │     │  + fans   │      │  + fans   │    │  + fans   │    │  + fans   │
        │  + 6×T    │     │  + 6×T    │      │  + 6×T    │    │  + 6×T    │    │  + 6×T    │
        └───────────┘     └───────────┘      └───────────┘    └───────────┘    └───────────┘
```

---

## 3. Bill of Materials — current state

### 3.1 Received (per the April 2026 RS order)

| Item | Part | Qty | Use |
|---|---|---|---|
| 24 V heater PSU | Meanwell **UHP-200-24** | 6 (5 + 1 spare) | One per module |
| 12 V auxiliary PSU | TRACOPOWER **TXN 35-112** | 6 | One per module (ESP32 + fans) |
| MCU | **ESP32-DEV-38P** (Seeit) | 3 | Chamber modules |
| MCU (alt) | **ESP32-DEV-30P** (Seeit) | 3 | Earmarked for chamber too (TBC) |
| Heater MOSFET (direct 3.3 V drive) | Infineon **IRLB8721PBF** TO-220 | 20 | Path A: drive from ESP32 directly |
| Heater MOSFET (driver-paired) | Infineon **IRFZ44NPBF** TO-220 | 20 | Path B: drive via MCP1407 |
| Gate driver | Microchip **MCP1407-E/P** DIP-8 | 20 | Pairs with IRFZ44N if needed |
| Fan | RND **460-00033** 120 × 25 mm 12 V, 2-wire | 24 | 2 per module + spares |
| Fan switch (alt 1) | Reuse IRLB8721 (MOSFET) | — | Preferred |
| Fan switch (alt 2) | Seeit **TTL-RELAY04** 4-ch relay board | 6 | Backup option if MOSFET path proves troublesome |
| Heater control sensor | DFRobot **DFR0198** DS18B20 waterproof | 6 | Closed-loop control of the NiChrome wire |
| NiChrome standoffs | Goodfellow ceramic beads 1200 °C, 4.5 mm | 2 packs | Wire suspension |
| Shielding | 3M 1182 antistatic copper foil tape, 19.1 mm × 16 m | 1 | EMI shielding around sensitive optics |
| Control cable | RS PRO shielded reels (98 m + 36 m + 623 m) | 1 each | Distribution + signal |
| Raspberry Pi 5 4 GB | Raspberry Pi | 2 | Central controller + spare |

### 3.2 To be sourced separately

| Item | Notes |
|---|---|
| **DS18B20 bare sensors × 25+** | For the Rytov/Cn² array (5 per module × 5 modules). Could not source single-element sensors without cap/wires at RS Spain; must order from another distributor (Mouser, Digikey, AliExpress for prototype). |
| **NiChrome wire** | NiChrome 80, 20 AWG, ~6 m total. Source from RS under "resistance wire" or specialised suppliers. |
| **Thermal fuses** | 85 °C axial cutoff (Thermodisc / Cantherm), one per module, inline with heater. **Safety-critical — do not power chamber without these.** |
| **Input fuses** | 10 A fast-blow + inline holder per module, on the 24 V rail. |
| **TO-220 heatsinks** | Clip-on, ~15–20 °C/W (Fischer FK237 or similar), one per heater MOSFET. |
| **Resistors** | 100 Ω (gate), 10 kΩ (pulldown), 4.7 kΩ (1-Wire pullup). Cheap assortment kit if not in stock. |
| **Flyback diodes** | 1N4148 or SS14, one per fan if using MOSFET switching. |
| **Connectors** | High-current ring lugs / WAGO 221 for heater wiring (8 A). JST or screw terminals for sensor buses. |
| **Enclosures** | Per-module electronics bay or shared central enclosure for PSUs + MCUs. Mechanical design TBD. |
| **Aluminum mounting plate** | UHP-200-24 requires aluminum backing (≥2 mm, ≥300 × 300 mm equivalent) to hit full rated output. Could be the enclosure wall itself. |

---

## 4. Per-module electrical design

### 4.1 Power rails

- **24 V rail (UHP-200-24):** heater only. Up to 8.4 A. Inline 10 A fuse + 85 °C thermal fuse on the heater leg.
- **12 V rail (TXN 35-112):** ESP32 (via onboard regulator) + 2× fans (~0.56 A total at startup). 35 W headroom is comfortable.

The two rails share GND (mandatory — MOSFET source and ESP32 ground must be common for gate drive to reference correctly). Tie 24 V and 12 V grounds at one point per module, star topology if multiple modules share a chassis.

### 4.2 Heater drive — two candidate paths

We bought parts for **both** so we can pick after prototype-bench testing.

**Path A — Direct 3.3 V drive (simpler):**
```
ESP32 GPIO ──[100 Ω]── G(IRLB8721)
                       │
                       └──[10 kΩ to GND]
                       D ── NiChrome ── +24 V
                       S ── GND
```
- One part, no driver chip. R_DS(on) at 3.3 V V_GS is ~30–40 mΩ → ~2 W dissipation at 8 A. Needs a small heatsink.
- Risk: marginal saturation, MOSFET runs warm.

**Path B — MCP1407 gate driver + IRFZ44N (robust):**
```
ESP32 GPIO ── IN(MCP1407)
              VDD ── +12 V (decouple: 100 nF + 10 µF)
              OUT ──[10 Ω]── G(IRFZ44N)
                              │
                              └──[10 kΩ to GND]
                              D ── NiChrome ── +24 V
                              S ── GND
```
- Drives gate to full 12 V → IRFZ44N at R_DS(on) ~17 mΩ → <1.2 W at 8 A. Cooler.
- One extra IC, one extra decoupling cap.

**Decision rule.** Bench-test Path A first on one module. If MOSFET case temperature stays < 70 °C at 8 A continuous with a small clip-on heatsink, ship Path A. Otherwise upgrade to Path B.

### 4.3 Fan drive

Two fans per module wired in parallel (≈0.56 A at 12 V), switched together.

Preferred: one IRLB8721 with 100 Ω gate / 10 kΩ pulldown, ESP32 GPIO direct. Flyback diode (SS14 or 1N4148) across each fan's V+/GND, cathode to +12 V.

Backup: TTL-RELAY04 board (5 V TTL input from ESP32 via level-shifter or via 3.3 V if board supports it — verify). Adds opto-isolation but introduces mechanical wear and audible clicking.

### 4.4 Temperature sensors

- **Heater control loop (received):** 1× DFR0198 (waterproof DS18B20 with cable) mounted near the wire to give a fast control feedback signal. Place close enough to track wire temperature, far enough to survive long-term.
- **Cn² array (to be ordered):** 5× bare DS18B20 in a vertical line on the side wall, spaced ~10 cm apart, on a separate 1-Wire bus. 4.7 kΩ pullup on the data line, 3-wire mode (VCC + GND + DATA).

Both buses use 3.3 V from the ESP32 LDO output (DS18B20 spec is 3.0–5.5 V).

### 4.5 ESP32 pin map (proposed, to be finalised in firmware/include/board_config.h)

| Function | GPIO | Notes |
|---|---|---|
| Heater PWM | GPIO 25 | LEDC channel 0, 100–500 Hz |
| Heater control sensor 1-Wire | GPIO 4 | DFR0198 |
| Cn² array 1-Wire | GPIO 5 | 5× DS18B20 |
| Fan switch | GPIO 26 | MOSFET gate |
| Status LED | GPIO 2 | Onboard blue LED |
| Thermal fuse status (optional) | GPIO 34 | Input-only, monitors if 85 °C fuse blew |
| Reserved (current sense ADC) | GPIO 35 | Future |

Avoid: GPIO 0, 1, 3 (boot/UART), GPIO 6–11 (SPI flash), GPIO 12 (boot strap), GPIO 15 (boot strap).

---

## 5. Firmware architecture (ESP32)

**Stack:** PlatformIO + Arduino framework (faster iteration; can drop to ESP-IDF later if needed).

**Libraries:**
- `OneWire` + `DallasTemperature` (DS18B20)
- `PubSubClient` or `AsyncMqttClient` (MQTT)
- `ArduinoJson` (telemetry payloads)
- `ESP32 LEDC` (heater PWM, built-in)
- Built-in WiFi

**Per-module state machine:**

```
IDLE ─── setpoint received ───▶ HEATING
                                  │
                                  ├── temp reached setpoint ───▶ STEADY_STATE
                                  ├── thermal fuse blown ──────▶ FAULT
                                  └── disconnect from MQTT ────▶ SAFE_OFF

STEADY_STATE ── purge command ───▶ PURGING ── purge done ────▶ SETTLING ── settled ──▶ READY ── back to STEADY_STATE
STEADY_STATE ── stop command ────▶ SAFE_OFF
FAULT       ── manual reset ─────▶ SAFE_OFF
```

**Safety invariants (enforced in firmware, regardless of MQTT state):**
- If heater control sensor reads > 90 °C → cut heater PWM to 0 immediately.
- If any of the 5 array sensors reads > 60 °C → cut heater PWM (PMMA wall protection).
- If MQTT disconnected for > 30 s → enter SAFE_OFF (heater 0 %, fans on, broadcast fault).
- If sensor reading fails (1-Wire error) for > 5 consecutive cycles → enter SAFE_OFF.
- On boot, heater output is always 0 until first valid setpoint received. **MOSFET gate must have hardware pulldown — never rely on firmware for boot-time safety.**

**Control loop:**
- PID on heater control sensor → PWM duty cycle.
- Sample period: 1 Hz (DS18B20 12-bit conversion takes ~750 ms).
- PID gains start as configurable constants; tune per module from step-response data.

**Telemetry cadence:**
- 1 Hz: all 6 temperatures, PWM duty, fan state, fault flags.
- 0.1 Hz: WiFi RSSI, uptime, free heap.

---

## 6. Orchestrator architecture (Raspberry Pi 5)

**Stack:** Python 3.11+, `asyncio`, `aiomqtt` (paho-mqtt async wrapper), `Mosquitto` broker (`apt install mosquitto`), Qt or web-based touch UI.

**Components:**

```
orchestrator/
├── src/
│   ├── orchestrator/
│   │   ├── __init__.py
│   │   ├── broker_config/        # mosquitto.conf, ACLs
│   │   ├── modules/              # per-module state, PID config, calibration
│   │   ├── control/              # purge sequencer, multi-module coordinator
│   │   ├── logging/              # HDF5 + CSV writers, rotation, metadata
│   │   ├── analysis/             # online Cn² from D_T(r), Rytov estimates
│   │   └── ui/                   # Qt or FastAPI + HTMX touch UI
│   └── tests/
```

**Key responsibilities:**
- Subscribe to all `chamber/module_+/+` topics, persist to HDF5/CSV.
- Publish setpoints, purge commands, and experiment recipes.
- Show live state of all 5 modules on the touchscreen.
- Compute Cn² in real time from the 5-sensor array per module.
- Tag log files with experiment metadata (operator, date, setup notes).

---

## 7. MQTT topic schema

All topics under `chamber/`. Module IDs are `m1` … `m5`.

### 7.1 ESP32 → Pi (telemetry)

| Topic | Payload (JSON) | Rate |
|---|---|---|
| `chamber/m{N}/temps` | `{"ctrl": 245.3, "array": [22.1, 23.4, 25.8, 28.9, 30.2], "ts": 1718000000}` | 1 Hz |
| `chamber/m{N}/status` | `{"state": "STEADY_STATE", "pwm": 0.42, "fan": false, "uptime_s": 3600, "rssi": -55}` | 1 Hz |
| `chamber/m{N}/fault` | `{"code": "OVERTEMP_ARRAY", "value": 62.3, "ts": ...}` | on event |

### 7.2 Pi → ESP32 (commands)

| Topic | Payload | Effect |
|---|---|---|
| `chamber/m{N}/cmd/setpoint` | `{"target_c": 250.0}` or `{"pwm_pct": 35}` | Set PID setpoint or open-loop duty |
| `chamber/m{N}/cmd/fan` | `{"on": true}` | Switch fans |
| `chamber/m{N}/cmd/purge` | `{"duration_s": 60, "settle_s": 30}` | Run purge sequence |
| `chamber/m{N}/cmd/stop` | `{}` | Immediate SAFE_OFF |
| `chamber/m{N}/cmd/config` | `{"pid": {"kp": 5.0, "ki": 0.1, "kd": 0.0}, "limits": {...}}` | Update PID and safety limits |

### 7.3 Broadcast

| Topic | Payload | Effect |
|---|---|---|
| `chamber/all/cmd/stop` | `{}` | All modules SAFE_OFF immediately. |
| `chamber/all/cmd/purge` | `{...}` | All modules purge together. |

QoS: 1 for commands, 0 for telemetry. Retain flag set on `status` and last `setpoint`.

---

## 8. Safety constraints (non-negotiable)

1. **Hardware-level over-temperature protection:** 85 °C axial thermal fuse in series with each heater. Bonded to PMMA wall in the worst-case hotspot. Independent of any firmware.
2. **MOSFET gate pulldown:** 10 kΩ G→S on every MOSFET. Heater output is OFF at boot, during ESP32 reset, and during firmware update.
3. **Per-module input fuse:** 10 A fast-blow on the 24 V rail, before any switching element.
4. **Firmware kill conditions:** see Section 5 (safety invariants).
5. **Mains safety:** all PSUs in an earthed enclosure. Strain relief on every AC entry. Class I installation, follow the UHP-200-24 datasheet ground-bonding instructions.
6. **NiChrome wire:** minimum 8 cm clearance to any PMMA surface. Suspend on ceramic beads. No direct contact ever.
7. **Operation:** never leave the chamber heated unattended without all software safety checks passing for at least 10 minutes. Touchscreen UI must show a clear "ARMED" indicator.

These rules apply during prototype bench testing as well as final operation.

---

## 9. Repository structure (proposed)

```
atmospheric-chamber/
├── README.md                    ← THIS FILE (links to docs/)
├── PROJECT_SEED.md              ← snapshot for Claude Code (this file copy)
├── docs/
│   ├── 01-overview.md
│   ├── 02-hardware-bom.md
│   ├── 03-electrical-design.md
│   ├── 04-safety.md
│   ├── 05-firmware-architecture.md
│   ├── 06-orchestrator-architecture.md
│   ├── 07-mqtt-schema.md
│   ├── 08-experimental-protocols.md
│   └── wiring-diagrams/         ← KiCad schematics, fritzing, PDF exports
├── firmware/
│   └── esp32-module/            ← PlatformIO project
│       ├── platformio.ini
│       ├── include/
│       │   └── board_config.h
│       ├── src/
│       │   ├── main.cpp
│       │   ├── heater.{cpp,h}
│       │   ├── sensors.{cpp,h}
│       │   ├── fan.{cpp,h}
│       │   ├── mqtt_client.{cpp,h}
│       │   ├── state_machine.{cpp,h}
│       │   └── safety.{cpp,h}
│       └── test/
├── orchestrator/                ← Pi 5 Python package
│   ├── pyproject.toml
│   ├── src/orchestrator/
│   └── tests/
├── analysis/                    ← Notebooks: Cn², Rytov, calibration
│   └── notebooks/
├── hardware/
│   ├── kicad/                   ← per-module PCB if/when we make one
│   ├── cad/                     ← FreeCAD/STEP of chamber + standoffs
│   └── bom/                     ← exported BOM CSVs
└── tools/
    ├── flash-all-modules.sh
    ├── log-replay.py
    └── calibrate-ds18b20.py
```

---

## 10. Roadmap — what to build first

**Phase 0 — bench validation (single module on the workbench, no chamber yet):**
1. Set up PlatformIO project skeleton with WiFi + MQTT echo.
2. Wire one UHP-200-24, one TXN 35-112, one ESP32, one MOSFET (Path A first), and a short NiChrome test load (e.g. a 5 Ω power resistor as proxy until NiChrome arrives).
3. Validate heater PWM under thermal load, monitor MOSFET temperature.
4. Wire one DFR0198 sensor and confirm 1-Wire reads.
5. Wire two fans, validate on/off switching.
6. Bring up Mosquitto on the Pi 5, confirm telemetry round-trip.

**Phase 1 — single-module integration:**
1. Build the safety state machine (overtemp cutoff, MQTT timeout SAFE_OFF).
2. Tune a basic PID against the DFR0198 sensor.
3. Implement the purge state machine.
4. Add HDF5 logging on the Pi.
5. Mechanical: design and 3D-print the NiChrome standoff frame, the sensor array mount, the electronics bay.

**Phase 2 — scale to 5 modules:**
1. Provisioning script (Wi-Fi creds, module ID, calibration constants per ESP32).
2. Touch UI with all 5 modules visible.
3. Coordinated multi-module purge.
4. Order and integrate the 25× bare DS18B20 for the Cn² array.

**Phase 3 — characterization:**
1. Calibrate each DS18B20 against a reference (NIST-traceable thermometer or ice/boiling bath).
2. Step-response per module → final PID gains.
3. First Cn² measurements at varying setpoints.
4. First FSO/VLC experiment through the full 5 m path.

---

## 11. Open decisions and notes

- **Fan switching: MOSFET vs relay.** Default MOSFET. Relay boards (TTL-RELAY04) kept as fallback. Firmware should abstract the "fan on/off" call so swapping the hardware is transparent.
- **MOSFET path: A vs B.** Decide after bench thermal test of Path A.
- **ESP32 variant mix:** 38-pin and 30-pin boards in stock. Firmware should compile for both; only pin mappings differ. Consider keeping the heater on a GPIO present on both variants.
- **Cable reels:** 98 m, 36 m, 623 m shielded reels received from RS. Cross-section / core count needs to be checked on each label before assignment to heater distribution vs sensor buses.
- **Aluminum mounting plate for UHP-200-24:** must be in place before sustained full-power operation, otherwise PSU derates significantly.
- **Touch UI framework:** Qt (PyQt6 or PySide6) vs FastAPI + HTMX in a kiosk Chromium. No decision yet — recommend trying FastAPI + HTMX first because it allows remote monitoring from any browser on the lab LAN without extra work.
- **Data logging format:** HDF5 with per-experiment files, metadata in attributes. CSV mirror for ad-hoc inspection. InfluxDB optional if we want live Grafana dashboards.

---

## 12. References

- Meanwell UHP-200-24 datasheet (in `docs/datasheets/`).
- TRACOPOWER TXN 35-112 datasheet.
- DS18B20 datasheet (Maxim/Analog Devices).
- IRLB8721PBF, IRFZ44N, MCP1407 datasheets.
- ESP32 LEDC API documentation.
- Andrews, L. C. & Phillips, R. L., *Laser Beam Propagation through Random Media* (for Cn² and Rytov variance theory).
- Tatarski, V. I., *Wave Propagation in a Turbulent Medium* (foundational, for D_T(r) → Cn²).

---

*Last updated: June 2026. Maintainer: Vicente (IDeTIC / ULPGC). Drop questions in issues, not in this file.*
