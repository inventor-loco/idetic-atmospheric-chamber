"""Tests for the in-memory module state mirror."""

from orchestrator.models import Status, Temps
from orchestrator.modules import ModuleManager


def test_modules_start_offline():
    mgr = ModuleManager()
    assert all(not s.online for s in mgr.all_snapshots())
    assert len(mgr.all_snapshots()) == 5


def test_update_marks_online_and_stores_payload():
    mgr = ModuleManager()
    mgr.update_temps("m1", Temps(ctrl=245.3, array=[22.1, 23.4, 25.8, 28.9, 30.2], ts=1))
    mgr.update_status("m1", Status(state="HEATING", pwm=0.42, fan=False, uptime_s=10, rssi=-55))

    snap = mgr.snapshot("m1")
    assert snap.online
    assert snap.temps.ctrl == 245.3
    assert snap.status.state == "HEATING"


def test_offline_after_timeout():
    mgr = ModuleManager(offline_after_s=0.0)
    mgr.update_temps("m2", Temps(ctrl=20.0, array=[20, 20, 20, 20, 20], ts=1))
    # With a zero timeout the module is immediately considered stale.
    assert not mgr.snapshot("m2").online
