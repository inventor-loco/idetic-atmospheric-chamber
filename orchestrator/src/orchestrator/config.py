"""Orchestrator runtime configuration."""

from __future__ import annotations

from pydantic import BaseModel


class BrokerConfig(BaseModel):
    host: str = "localhost"
    port: int = 1883
    username: str | None = None
    password: str | None = None


class LoggingConfig(BaseModel):
    output_dir: str = "./data"
    hdf5: bool = True
    csv_mirror: bool = True


class OtaConfig(BaseModel):
    # Directory of firmware images, named "<version>.bin".
    images_dir: str = "./firmware-images"
    # Interface/port the firmware HTTP server binds to on the Pi.
    http_host: str = "0.0.0.0"
    http_port: int = 8266
    # Host/IP the ESP32s should fetch from (the Pi's address on the lab LAN).
    # Must be reachable from the modules — usually the Pi's static LAN IP.
    advertise_host: str = "192.168.1.10"
    # Per-module rollout timeout: download + reboot + reconnect + version report.
    rollout_timeout_s: float = 180.0


class OrchestratorConfig(BaseModel):
    broker: BrokerConfig = BrokerConfig()
    logging: LoggingConfig = LoggingConfig()
    ota: OtaConfig = OtaConfig()
    # Offline threshold: a module is marked offline if no telemetry within this.
    module_offline_s: float = 5.0
