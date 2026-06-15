"""In-memory mirror of all five modules' last-known state.

The MQTT bridge feeds decoded telemetry in via ``update_*``; the UI and control
layers read snapshots out. No I/O here — this is pure state so it stays easy to
unit-test.
"""

from __future__ import annotations

import time

from ..models import Fault, ModuleSnapshot, Status, Temps
from ..topics import MODULE_IDS


class ModuleManager:
    def __init__(self, offline_after_s: float = 5.0) -> None:
        self._offline_after_s = offline_after_s
        self._modules: dict[str, ModuleSnapshot] = {
            mid: ModuleSnapshot(module_id=mid) for mid in MODULE_IDS
        }
        self._last_seen: dict[str, float] = {}

    def update_temps(self, module_id: str, temps: Temps) -> None:
        self._modules[module_id].temps = temps
        self._touch(module_id)

    def update_status(self, module_id: str, status: Status) -> None:
        self._modules[module_id].status = status
        self._touch(module_id)

    def update_fault(self, module_id: str, fault: Fault) -> None:
        self._modules[module_id].last_fault = fault
        self._touch(module_id)

    def _touch(self, module_id: str) -> None:
        self._last_seen[module_id] = time.monotonic()
        self._modules[module_id].online = True

    def snapshot(self, module_id: str) -> ModuleSnapshot:
        self._refresh_online(module_id)
        return self._modules[module_id]

    def all_snapshots(self) -> list[ModuleSnapshot]:
        return [self.snapshot(mid) for mid in MODULE_IDS]

    def _refresh_online(self, module_id: str) -> None:
        last = self._last_seen.get(module_id)
        if last is None or (time.monotonic() - last) > self._offline_after_s:
            self._modules[module_id].online = False
