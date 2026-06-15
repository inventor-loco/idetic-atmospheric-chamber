"""OTA rollout orchestration.

Drives a **safety-gated, sequential** rollout: one module at a time, each only
if its heater is confirmed off, with per-module verification that the module
came back up running the new firmware version. Sequential-by-design so a bad
image cannot brick all five modules simultaneously.
"""

from __future__ import annotations

import asyncio
import logging
import time
from dataclasses import dataclass

from ..config import OtaConfig
from ..models import OtaCmd
from ..modules import ModuleManager
from ..mqtt_bridge import MqttBridge
from ..topics import MODULE_IDS
from .server import FirmwareStore

log = logging.getLogger(__name__)

# States in which the heater is guaranteed off, so an OTA + reboot is safe.
# Mirrors otaAllowed() in firmware/esp32-module/src/main.cpp.
SAFE_STATES = {"IDLE", "SAFE_OFF", "READY", "FAULT"}


@dataclass
class RolloutResult:
    module_id: str
    ok: bool
    message: str
    from_version: str | None = None
    to_version: str | None = None


class OtaManager:
    def __init__(
        self,
        store: FirmwareStore,
        config: OtaConfig,
        bridge: MqttBridge,
        manager: ModuleManager,
    ) -> None:
        self._store = store
        self._config = config
        self._bridge = bridge
        self._manager = manager

    async def rollout(
        self, version: str, module_ids: list[str] | None = None
    ) -> list[RolloutResult]:
        """Update the given modules (default: all) to ``version``, in order.

        Stops at the first failure so a broken image doesn't propagate.
        """
        image = self._store.get(version)  # raises if missing
        targets = module_ids or MODULE_IDS
        results: list[RolloutResult] = []
        for module_id in targets:
            result = await self.update_one(module_id, version, image_md5=image.md5)
            results.append(result)
            if not result.ok:
                log.error("rollout halted: %s failed (%s)", module_id, result.message)
                break
        return results

    async def update_one(
        self, module_id: str, version: str, *, image_md5: str | None = None
    ) -> RolloutResult:
        if module_id not in MODULE_IDS:
            return RolloutResult(module_id, False, f"unknown module {module_id!r}")

        image = self._store.get(version)
        if image_md5 and image.md5 != image_md5:
            return RolloutResult(module_id, False, "image changed mid-rollout")

        snap = self._manager.snapshot(module_id)
        current_fw = snap.fw

        if not snap.online:
            return RolloutResult(module_id, False, "module offline", current_fw, version)
        if snap.fw == version:
            return RolloutResult(module_id, True, "already up to date", version, version)
        state = snap.status.state if snap.status else None
        if state not in SAFE_STATES:
            return RolloutResult(
                module_id, False,
                f"unsafe state {state!r}; stop/idle the module first",
                current_fw, version,
            )

        # Fire the command and wait for the module to confirm the new version.
        self._manager.reset_ota(module_id)
        cmd = OtaCmd(
            url=image.url(self._config.advertise_host, self._config.http_port),
            version=version,
            md5=image.md5,
            size=image.size,
        )
        log.info("OTA %s -> %s (%s)", module_id, version, cmd.url)
        await self._bridge.send_ota(module_id, cmd)

        return await self._await_completion(module_id, version, current_fw)

    async def _await_completion(
        self, module_id: str, version: str, from_version: str | None
    ) -> RolloutResult:
        deadline = time.monotonic() + self._config.rollout_timeout_s
        while time.monotonic() < deadline:
            snap = self._manager.snapshot(module_id)

            ota = snap.last_ota
            if ota and ota.version == version and ota.phase in ("FAILED", "REJECTED"):
                return RolloutResult(
                    module_id, False,
                    f"{ota.phase.lower()}: {ota.error or 'no detail'}",
                    from_version, version,
                )

            # Success is confirmed only when the module reboots and reports the
            # new running firmware version — the ground truth, not a claim.
            if snap.online and snap.fw == version:
                return RolloutResult(module_id, True, "updated", from_version, version)

            await asyncio.sleep(0.5)

        return RolloutResult(
            module_id, False, "timed out waiting for new version", from_version, version
        )
