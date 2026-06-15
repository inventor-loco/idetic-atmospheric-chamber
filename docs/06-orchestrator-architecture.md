# 06 — Orchestrator Architecture (Raspberry Pi 5)

Detail in [`PROJECT_SEED.md` §6](../PROJECT_SEED.md). Code:
[`orchestrator/`](../orchestrator/).

## Stack
Python 3.11+, `asyncio`, `aiomqtt`, Mosquitto broker (`apt install mosquitto`),
touch UI (FastAPI + HTMX leaning, PyQt6 alternative — seed §11).

## Module layout
| Module | Responsibility |
|---|---|
| `topics.py` | MQTT topic schema (mirror of seed §7) |
| `models.py` | pydantic telemetry/command models + `ModuleSnapshot` |
| `config.py` | broker / logging / runtime config |
| `mqtt_bridge.py` | async subscribe → decode → `ModuleManager`; typed command publishers |
| `modules/manager.py` | in-memory mirror of all 5 modules (pure, testable) |
| `control/` | purge sequencer, multi-module coordinator |
| `logging/writer.py` | HDF5 (+ CSV mirror) experiment logger |
| `analysis/cn2.py` | online Cn² from the temperature structure function |
| `ui/` | touchscreen UI (thin view over manager + bridge) |
| `broker_config/mosquitto.conf` | broker deployment config |

## Run
```bash
cd orchestrator
pip install -e .[dev]
python -m orchestrator        # connects to broker, mirrors telemetry
pytest                        # unit tests
```

## Responsibilities
Subscribe to `chamber/+/+`, persist to HDF5/CSV, publish setpoints/purges/recipes,
show live module state, compute Cn² in real time, tag logs with experiment metadata.
