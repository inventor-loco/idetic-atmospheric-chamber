"""Telemetry and command payload models (PROJECT_SEED §7).

Pydantic models give us validation on the wire boundary and a typed mirror of
each module's last-known state.
"""

from __future__ import annotations

from typing import Literal

from pydantic import BaseModel, Field

State = Literal[
    "IDLE", "HEATING", "STEADY_STATE", "PURGING",
    "SETTLING", "READY", "SAFE_OFF", "FAULT",
]


class Temps(BaseModel):
    """`chamber/m{N}/temps` payload."""

    ctrl: float                       # heater control sensor, °C
    array: list[float]                # 5-element vertical array, bottom→top, °C
    ts: int                           # epoch seconds


class Status(BaseModel):
    """`chamber/m{N}/status` payload (retained)."""

    state: State
    pwm: float = Field(ge=0.0, le=1.0)
    fan: bool
    uptime_s: int
    rssi: int


class Fault(BaseModel):
    """`chamber/m{N}/fault` payload."""

    code: str
    value: float
    ts: int


# --- Commands ----------------------------------------------------------------
class Setpoint(BaseModel):
    target_c: float | None = None
    pwm_pct: float | None = None


class FanCmd(BaseModel):
    on: bool


class PurgeCmd(BaseModel):
    duration_s: int = 60
    settle_s: int = 30


class PidGains(BaseModel):
    kp: float
    ki: float
    kd: float


class ConfigCmd(BaseModel):
    pid: PidGains
    limits: dict[str, float] = Field(default_factory=dict)


class ModuleSnapshot(BaseModel):
    """Last-known full state of one module, held in memory by the manager."""

    module_id: str
    temps: Temps | None = None
    status: Status | None = None
    last_fault: Fault | None = None
    online: bool = False
