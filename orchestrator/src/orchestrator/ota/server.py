"""Firmware image store + HTTP server.

Images live in a directory named ``<version>.bin`` (e.g. ``0.2.0.bin``). The
store computes the MD5 and size the firmware needs for its integrity check; the
server exposes the directory over plain HTTP on the lab LAN so each ESP32 can
GET its image.
"""

from __future__ import annotations

import hashlib
import logging
from dataclasses import dataclass
from pathlib import Path

from aiohttp import web

from ..config import OtaConfig

log = logging.getLogger(__name__)


@dataclass(frozen=True)
class FirmwareImage:
    version: str
    path: Path
    md5: str
    size: int

    def url(self, advertise_host: str, port: int) -> str:
        return f"http://{advertise_host}:{port}/firmware/{self.path.name}"


class FirmwareStore:
    """Resolves firmware versions to on-disk images and their checksums."""

    def __init__(self, images_dir: str | Path) -> None:
        self.images_dir = Path(images_dir)

    def available(self) -> list[str]:
        if not self.images_dir.is_dir():
            return []
        return sorted(p.stem for p in self.images_dir.glob("*.bin"))

    def get(self, version: str) -> FirmwareImage:
        path = self.images_dir / f"{version}.bin"
        if not path.is_file():
            raise FileNotFoundError(
                f"firmware {version!r} not found in {self.images_dir} "
                f"(have: {self.available()})"
            )
        data = path.read_bytes()
        return FirmwareImage(
            version=version,
            path=path,
            md5=hashlib.md5(data).hexdigest(),
            size=len(data),
        )


class FirmwareServer:
    """Async static HTTP server for firmware images."""

    def __init__(self, config: OtaConfig) -> None:
        self._config = config
        self._store = FirmwareStore(config.images_dir)
        self._runner: web.AppRunner | None = None

    @property
    def store(self) -> FirmwareStore:
        return self._store

    async def start(self) -> None:
        Path(self._config.images_dir).mkdir(parents=True, exist_ok=True)
        app = web.Application()
        app.router.add_static("/firmware/", self._config.images_dir)
        self._runner = web.AppRunner(app)
        await self._runner.setup()
        site = web.TCPSite(self._runner, self._config.http_host, self._config.http_port)
        await site.start()
        log.info(
            "firmware server on http://%s:%d/firmware/ (advertising %s)",
            self._config.http_host, self._config.http_port, self._config.advertise_host,
        )

    async def stop(self) -> None:
        if self._runner is not None:
            await self._runner.cleanup()
            self._runner = None
