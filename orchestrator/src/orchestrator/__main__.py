"""Orchestrator entry point.

Phase-0 skeleton: connects to the broker, mirrors all module telemetry into the
ModuleManager, and logs a 1 Hz heartbeat of online modules. The UI and logging
wiring land in Phase 1.
"""

from __future__ import annotations

import asyncio
import logging

from .config import OrchestratorConfig
from .modules import ModuleManager
from .mqtt_bridge import MqttBridge


async def _heartbeat(manager: ModuleManager) -> None:
    log = logging.getLogger("heartbeat")
    while True:
        online = [s.module_id for s in manager.all_snapshots() if s.online]
        log.info("online modules: %s", online or "none")
        await asyncio.sleep(1.0)


async def _run(config: OrchestratorConfig) -> None:
    manager = ModuleManager(offline_after_s=config.module_offline_s)
    bridge = MqttBridge(config.broker, manager)
    await asyncio.gather(bridge.run(), _heartbeat(manager))


def main() -> None:
    logging.basicConfig(level=logging.INFO,
                        format="%(asctime)s %(name)s %(levelname)s %(message)s")
    config = OrchestratorConfig()  # TODO: load from YAML / env in Phase 1
    try:
        asyncio.run(_run(config))
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
