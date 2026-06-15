#!/usr/bin/env python3
"""Stage a built firmware binary into the OTA image store.

Copies the PlatformIO build artifact to ``<images_dir>/<version>.bin`` so the
orchestrator's firmware server can serve it and OtaManager can roll it out.

Example:
    python tools/publish-firmware.py --version 0.2.0 --env esp32dev_38p
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DEFAULT_IMAGES = REPO / "orchestrator" / "firmware-images"


def build_artifact(env: str) -> Path:
    return REPO / "firmware" / "esp32-module" / ".pio" / "build" / env / "firmware.bin"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True, help="version string, e.g. 0.2.0")
    parser.add_argument("--env", default="esp32dev_38p", help="PlatformIO env name")
    parser.add_argument("--images-dir", type=Path, default=DEFAULT_IMAGES)
    parser.add_argument("--bin", type=Path, help="override path to firmware.bin")
    args = parser.parse_args()

    src = args.bin or build_artifact(args.env)
    if not src.is_file():
        raise SystemExit(f"firmware binary not found: {src}\n(run `pio run -e {args.env}` first)")

    args.images_dir.mkdir(parents=True, exist_ok=True)
    dst = args.images_dir / f"{args.version}.bin"
    shutil.copy2(src, dst)

    data = dst.read_bytes()
    print(f"published {dst}")
    print(f"  size: {len(data)} bytes")
    print(f"  md5:  {hashlib.md5(data).hexdigest()}")
    print("\nReminder: bump CHAMBER_FW_VERSION in platformio.ini to match the version,")
    print("so modules report the new version and OTA success can be confirmed.")


if __name__ == "__main__":
    main()
