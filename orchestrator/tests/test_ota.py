"""Tests for the OTA firmware store and rollout safety/verification logic."""

import hashlib

import pytest

from orchestrator.config import OtaConfig
from orchestrator.models import OtaStatus, Status
from orchestrator.modules import ModuleManager
from orchestrator.ota import FirmwareStore, OtaManager
from orchestrator.ota.manager import RolloutResult


def _write_image(images_dir, version: str, payload: bytes) -> str:
    images_dir.mkdir(parents=True, exist_ok=True)
    (images_dir / f"{version}.bin").write_bytes(payload)
    return hashlib.md5(payload).hexdigest()


# --- FirmwareStore -----------------------------------------------------------
def test_store_computes_md5_and_size(tmp_path):
    payload = b"\xe9firmware-bytes"
    md5 = _write_image(tmp_path, "0.2.0", payload)
    store = FirmwareStore(tmp_path)

    image = store.get("0.2.0")
    assert image.md5 == md5
    assert image.size == len(payload)
    assert image.url("192.168.1.10", 8266) == "http://192.168.1.10:8266/firmware/0.2.0.bin"
    assert store.available() == ["0.2.0"]


def test_store_missing_version_raises(tmp_path):
    store = FirmwareStore(tmp_path)
    with pytest.raises(FileNotFoundError):
        store.get("9.9.9")


# --- OtaManager safety gate --------------------------------------------------
class FakeBridge:
    """Stands in for MqttBridge; records the OTA command and can simulate the
    module's response by mutating the ModuleManager."""

    def __init__(self, manager: ModuleManager, on_send=None):
        self.manager = manager
        self.on_send = on_send
        self.sent = []

    async def send_ota(self, module_id, cmd):
        self.sent.append((module_id, cmd))
        if self.on_send:
            self.on_send(module_id, cmd)


def _manager_with_state(state: str, fw: str) -> ModuleManager:
    mgr = ModuleManager()
    mgr.update_status("m1", Status(state=state, pwm=0.0, fan=False, uptime_s=1, rssi=-50, fw=fw))
    return mgr


def _config() -> OtaConfig:
    return OtaConfig(rollout_timeout_s=2.0)


@pytest.fixture
def image(tmp_path):
    _write_image(tmp_path, "0.2.0", b"new-image")
    return FirmwareStore(tmp_path)


async def test_rejects_offline_module(image):
    mgr = ModuleManager()  # m1 never reported -> offline
    ota = OtaManager(image, _config(), FakeBridge(mgr), mgr)
    result = await ota.update_one("m1", "0.2.0")
    assert not result.ok and "offline" in result.message


async def test_rejects_unsafe_state(image):
    mgr = _manager_with_state("HEATING", "0.1.0")
    bridge = FakeBridge(mgr)
    ota = OtaManager(image, _config(), bridge, mgr)
    result = await ota.update_one("m1", "0.2.0")
    assert not result.ok and "unsafe" in result.message
    assert bridge.sent == []  # never published a command to a hot module


async def test_already_up_to_date_is_noop(image):
    mgr = _manager_with_state("IDLE", "0.2.0")
    ota = OtaManager(image, _config(), FakeBridge(mgr), mgr)
    result = await ota.update_one("m1", "0.2.0")
    assert result.ok and "already" in result.message


async def test_success_confirmed_by_new_version(image):
    mgr = _manager_with_state("SAFE_OFF", "0.1.0")

    def simulate_reboot(module_id, cmd):
        # Module flashes and comes back reporting the new firmware version.
        mgr.update_status(module_id,
                          Status(state="IDLE", pwm=0.0, fan=False,
                                 uptime_s=1, rssi=-50, fw=cmd.version))

    ota = OtaManager(image, _config(), FakeBridge(mgr, simulate_reboot), mgr)
    result = await ota.update_one("m1", "0.2.0")
    assert result == RolloutResult("m1", True, "updated", "0.1.0", "0.2.0")


async def test_failure_reported_by_module(image):
    mgr = _manager_with_state("SAFE_OFF", "0.1.0")

    def simulate_failure(module_id, cmd):
        mgr.update_ota(module_id,
                       OtaStatus(phase="FAILED", version=cmd.version, error="md5"))

    ota = OtaManager(image, _config(), FakeBridge(mgr, simulate_failure), mgr)
    result = await ota.update_one("m1", "0.2.0")
    assert not result.ok and "md5" in result.message


async def test_rollout_halts_on_first_failure(image):
    mgr = _manager_with_state("SAFE_OFF", "0.1.0")
    # m2..m5 are offline; m1 will fail -> rollout must stop after m1.
    def simulate_failure(module_id, cmd):
        mgr.update_ota(module_id, OtaStatus(phase="FAILED", version=cmd.version, error="x"))

    ota = OtaManager(image, _config(), FakeBridge(mgr, simulate_failure), mgr)
    results = await ota.rollout("0.2.0", ["m1", "m2"])
    assert len(results) == 1 and not results[0].ok
