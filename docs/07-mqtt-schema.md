# 07 — MQTT Topic Schema

Authoritative copy: [`PROJECT_SEED.md` §7](../PROJECT_SEED.md). Code mirrors:
[`orchestrator/src/orchestrator/topics.py`](../orchestrator/src/orchestrator/topics.py)
and
[`firmware/esp32-module/src/mqtt_client.cpp`](../firmware/esp32-module/src/mqtt_client.cpp).
**Keep all three in sync.**

All topics under `chamber/`. Module IDs `m1`…`m5`.

## ESP32 → Pi (telemetry)
| Topic | Payload | Rate |
|---|---|---|
| `chamber/m{N}/temps` | `{"ctrl":245.3,"array":[22.1,23.4,25.8,28.9,30.2],"ts":...}` | 1 Hz |
| `chamber/m{N}/status` | `{"state":"STEADY_STATE","pwm":0.42,"fan":false,"uptime_s":3600,"rssi":-55}` | 1 Hz, retained |
| `chamber/m{N}/fault` | `{"code":"OVERTEMP_ARRAY","value":62.3,"ts":...}` | on event |

## Pi → ESP32 (commands), QoS 1
| Topic | Payload | Effect |
|---|---|---|
| `chamber/m{N}/cmd/setpoint` | `{"target_c":250.0}` or `{"pwm_pct":35}` | PID setpoint or open-loop duty |
| `chamber/m{N}/cmd/fan` | `{"on":true}` | switch fans |
| `chamber/m{N}/cmd/purge` | `{"duration_s":60,"settle_s":30}` | purge sequence |
| `chamber/m{N}/cmd/stop` | `{}` | immediate SAFE_OFF |
| `chamber/m{N}/cmd/config` | `{"pid":{...},"limits":{...}}` | update PID + limits |

## Broadcast
| Topic | Effect |
|---|---|
| `chamber/all/cmd/stop` | all modules SAFE_OFF |
| `chamber/all/cmd/purge` | all modules purge together |

QoS: 1 commands, 0 telemetry. Retain on `status` and last `setpoint`.
