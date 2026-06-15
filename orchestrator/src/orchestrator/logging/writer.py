"""Experiment logger — one HDF5 file per experiment with metadata attributes,
plus an optional CSV mirror for ad-hoc inspection (PROJECT_SEED §6, §11).

Skeleton: the interface is real; the HDF5/CSV bodies are intentionally minimal
and marked TODO where dataset layout still needs deciding in Phase 1.
"""

from __future__ import annotations

from datetime import datetime, timezone
from pathlib import Path

from ..models import Temps


class ExperimentLogger:
    def __init__(self, output_dir: str | Path, experiment_name: str) -> None:
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        self.path = self.output_dir / f"{experiment_name}_{stamp}.h5"
        self._metadata: dict[str, str] = {}

    def set_metadata(self, **kwargs: str) -> None:
        """Operator, setup notes, etc. — written as HDF5 root attributes."""
        self._metadata.update({k: str(v) for k, v in kwargs.items()})

    def log_temps(self, module_id: str, temps: Temps) -> None:
        """Append one telemetry sample for a module."""
        # TODO(Phase 1): open the HDF5 file lazily, create a per-module group
        # with resizable datasets [ts, ctrl, array[5]], append here, flush
        # periodically. Mirror to CSV if csv_mirror is enabled.
        raise NotImplementedError

    def close(self) -> None:
        """Flush and finalize the file, writing metadata attributes."""
        # TODO(Phase 1): write self._metadata as root attrs and close handle.
        ...
