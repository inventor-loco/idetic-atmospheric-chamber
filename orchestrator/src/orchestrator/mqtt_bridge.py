"""Async MQTT bridge between the broker and the in-memory module manager.

Subscribes to all chamber telemetry, decodes payloads into models, feeds the
ModuleManager, and exposes typed command publishers (PROJECT_SEED §6, §7).
"""

from __future__ import annotations

import json
import logging

import aiomqtt

from . import topics
from .config import BrokerConfig
from .models import (
    ConfigCmd,
    FanCmd,
    Fault,
    PurgeCmd,
    Setpoint,
    Status,
    Temps,
)
from .modules import ModuleManager

log = logging.getLogger(__name__)


class MqttBridge:
    def __init__(self, broker: BrokerConfig, manager: ModuleManager) -> None:
        self._broker = broker
        self._manager = manager
        self._client: aiomqtt.Client | None = None

    async def run(self) -> None:
        """Connect and process telemetry forever. Reconnects on drop."""
        async with aiomqtt.Client(
            hostname=self._broker.host,
            port=self._broker.port,
            username=self._broker.username,
            password=self._broker.password,
        ) as client:
            self._client = client
            await client.subscribe(topics.TELEMETRY_WILDCARD, qos=topics.QOS_TELEMETRY)
            log.info("subscribed to %s", topics.TELEMETRY_WILDCARD)
            async for message in client.messages:
                self._dispatch(str(message.topic), message.payload)

    def _dispatch(self, topic: str, payload: bytes) -> None:
        parts = topic.split("/")
        if len(parts) < 3 or parts[0] != "chamber":
            return
        module_id, kind = parts[1], parts[2]
        if module_id not in topics.MODULE_IDS:
            return
        try:
            data = json.loads(payload)
        except (ValueError, TypeError):
            log.warning("bad payload on %s", topic)
            return

        if kind == "temps":
            self._manager.update_temps(module_id, Temps(**data))
        elif kind == "status":
            self._manager.update_status(module_id, Status(**data))
        elif kind == "fault":
            self._manager.update_fault(module_id, Fault(**data))

    # --- command publishers --------------------------------------------------
    async def _publish(self, topic: str, payload: dict) -> None:
        if self._client is None:
            raise RuntimeError("bridge not connected")
        await self._client.publish(topic, json.dumps(payload), qos=topics.QOS_COMMAND)

    async def set_setpoint(self, module_id: str, sp: Setpoint) -> None:
        await self._publish(topics.cmd_setpoint(module_id),
                            sp.model_dump(exclude_none=True))

    async def set_fan(self, module_id: str, cmd: FanCmd) -> None:
        await self._publish(topics.cmd_fan(module_id), cmd.model_dump())

    async def purge(self, module_id: str, cmd: PurgeCmd) -> None:
        await self._publish(topics.cmd_purge(module_id), cmd.model_dump())

    async def configure(self, module_id: str, cmd: ConfigCmd) -> None:
        await self._publish(topics.cmd_config(module_id), cmd.model_dump())

    async def stop(self, module_id: str) -> None:
        await self._publish(topics.cmd_stop(module_id), {})

    async def stop_all(self) -> None:
        """Broadcast emergency stop to every module (PROJECT_SEED §7.3)."""
        await self._publish(topics.ALL_STOP, {})
