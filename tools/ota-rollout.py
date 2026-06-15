#!/usr/bin/env python3
"""Drive an OTA rollout from the command line.

Self-contained: starts the firmware HTTP server, mirrors module telemetry off
the broker, then rolls the chosen version out to the target modules one at a
time, halting on the first failure.

Examples:
    python tools/ota-rollout.py 0.2.0                 # all modules
    python tools/ota-rollout.py 0.2.0 --modules m1 m2
    python tools/ota-rollout.py --list                # show available images

Run this on the Pi (or any host reachable by the modules); set --advertise-host
to the address the ESP32s should fetch from if it isn't the default.
"""

from __future__ import annotations

import argparse
import asyncio
import logging

from orchestrator.config import BrokerConfig, OtaConfig
from orchestrator.modules import ModuleManager
from orchestrator.mqtt_bridge import MqttBridge
from orchestrator.ota import FirmwareServer, OtaManager


async def _run(args: argparse.Namespace) -> int:
    ota_cfg = OtaConfig(
        images_dir=args.images_dir,
        http_port=args.http_port,
        advertise_host=args.advertise_host,
    )
    server = FirmwareServer(ota_cfg)

    if args.list:
        print("available firmware images:", server.store.available() or "(none)")
        return 0

    manager = ModuleManager()
    bridge = MqttBridge(BrokerConfig(host=args.broker, port=args.port), manager)

    await server.start()
    bridge_task = asyncio.create_task(bridge.run())
    # Give the telemetry mirror a moment to receive retained status messages so
    # the safety gate sees each module's real state before we send commands.
    await asyncio.sleep(args.warmup_s)

    ota = OtaManager(server.store, ota_cfg, bridge, manager)
    try:
        results = await ota.rollout(args.version, args.modules or None)
    finally:
        bridge_task.cancel()
        await server.stop()

    print("\nRollout results:")
    failed = False
    for r in results:
        flag = "OK " if r.ok else "FAIL"
        print(f"  [{flag}] {r.module_id}: {r.from_version} -> {r.to_version}  {r.message}")
        failed |= not r.ok
    return 1 if failed else 0


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", nargs="?", help="firmware version to roll out")
    parser.add_argument("--modules", nargs="*", help="module ids (default: all)")
    parser.add_argument("--list", action="store_true", help="list available images and exit")
    parser.add_argument("--broker", default="localhost")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--images-dir", default="./firmware-images")
    parser.add_argument("--http-port", type=int, default=8266)
    parser.add_argument("--advertise-host", default="192.168.1.10")
    parser.add_argument("--warmup-s", type=float, default=3.0)
    args = parser.parse_args()

    if not args.list and not args.version:
        parser.error("version is required unless --list is given")

    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    raise SystemExit(asyncio.run(_run(args)))


if __name__ == "__main__":
    main()
