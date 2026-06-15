"""MQTT topic schema — the single source of truth, mirrors PROJECT_SEED §7.

The firmware (firmware/esp32-module/src/mqtt_client.cpp) must stay in sync with
this module. Module IDs are ``m1`` … ``m5``.
"""

from __future__ import annotations

MODULE_IDS = [f"m{n}" for n in range(1, 6)]

# --- ESP32 -> Pi (telemetry) -------------------------------------------------
def temps(module_id: str) -> str:
    return f"chamber/{module_id}/temps"


def status(module_id: str) -> str:
    return f"chamber/{module_id}/status"


def fault(module_id: str) -> str:
    return f"chamber/{module_id}/fault"


# --- Pi -> ESP32 (commands) --------------------------------------------------
def cmd_setpoint(module_id: str) -> str:
    return f"chamber/{module_id}/cmd/setpoint"


def cmd_fan(module_id: str) -> str:
    return f"chamber/{module_id}/cmd/fan"


def cmd_purge(module_id: str) -> str:
    return f"chamber/{module_id}/cmd/purge"


def cmd_stop(module_id: str) -> str:
    return f"chamber/{module_id}/cmd/stop"


def cmd_config(module_id: str) -> str:
    return f"chamber/{module_id}/cmd/config"


# --- Broadcast ---------------------------------------------------------------
ALL_STOP = "chamber/all/cmd/stop"
ALL_PURGE = "chamber/all/cmd/purge"

# Subscription wildcard covering all telemetry from all modules.
TELEMETRY_WILDCARD = "chamber/+/+"

# QoS policy (PROJECT_SEED §7): commands reliable, telemetry fire-and-forget.
QOS_COMMAND = 1
QOS_TELEMETRY = 0
