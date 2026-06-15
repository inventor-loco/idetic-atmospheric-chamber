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


class OrchestratorConfig(BaseModel):
    broker: BrokerConfig = BrokerConfig()
    logging: LoggingConfig = LoggingConfig()
    # Offline threshold: a module is marked offline if no telemetry within this.
    module_offline_s: float = 5.0
