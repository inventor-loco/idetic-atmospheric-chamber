#!/usr/bin/env bash
# Flash the module firmware to every connected ESP32.
#
# Usage: tools/flash-all-modules.sh [env]
#   env defaults to esp32dev_38p (see firmware/esp32-module/platformio.ini).
#
# Each board needs its own MODULE_ID (m1..m5) in secrets.ini before flashing.
# For provisioning many boards, override per-port (Phase 2 provisioning script).
set -euo pipefail

ENV="${1:-esp32dev_38p}"
FW_DIR="$(dirname "$0")/../firmware/esp32-module"

cd "$FW_DIR"
echo "Building + uploading env=$ENV to all detected serial ports..."
# pio detects the port automatically when a single board is attached.
# TODO(Phase 2): loop over `pio device list` and map ports -> module IDs.
pio run -e "$ENV" --target upload
