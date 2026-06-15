# 08 — Experimental Protocols

Procedures for running characterization and FSO/VLC/OCC experiments. To be
filled in during Phase 3 (see [`PROJECT_SEED.md` §10](../PROJECT_SEED.md)).

## Planned protocols
- **DS18B20 calibration** — two-point (ice / boiling) per sensor, against a
  reference; see [`tools/calibrate-ds18b20.py`](../tools/calibrate-ds18b20.py).
- **Step-response / PID tuning** — per module, derive final PID gains from the
  heater step response.
- **Cn² vs setpoint** — sweep heater setpoints, record the vertical array,
  compute Cn² via [`analysis/cn2.py`](../orchestrator/src/orchestrator/analysis/cn2.py),
  cross-check against Rytov variance.
- **Full-path FSO/VLC** — propagate through the 5 m path at controlled
  turbulence levels; correlate link metrics with Cn².

## Scientific targets (seed §1)
- Heater surface 300–400 °C; air ΔT 30–60 K; PMMA wall < 60 °C.
- Multi-hour unattended operation with all safety checks passing.

Notebooks: [`analysis/notebooks/`](../analysis/notebooks/).
