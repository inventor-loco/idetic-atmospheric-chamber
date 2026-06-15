"""Over-the-air firmware update orchestration.

The Pi serves firmware binaries over HTTP (:class:`FirmwareServer`) and drives a
safety-gated, sequential rollout to the ESP32 modules (:class:`OtaManager`).
Modules pull the image, verify its MD5, flash the inactive OTA partition, and
reboot — see firmware/esp32-module/src/ota.cpp.
"""

from .manager import OtaManager, RolloutResult
from .server import FirmwareImage, FirmwareServer, FirmwareStore

__all__ = [
    "FirmwareImage",
    "FirmwareServer",
    "FirmwareStore",
    "OtaManager",
    "RolloutResult",
]
