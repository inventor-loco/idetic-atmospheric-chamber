#!/usr/bin/env python3
"""Replay a recorded experiment HDF5/CSV file back onto the MQTT bus.

Useful for exercising the UI and analysis without the hardware powered. Stub:
wire up once the HDF5 dataset layout is fixed in Phase 1.
"""

from __future__ import annotations

import argparse


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logfile", help="experiment .h5 or .csv to replay")
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--speed", type=float, default=1.0, help="playback rate multiplier")
    parser.parse_args()
    raise SystemExit("TODO(Phase 1): implement replay once log format is fixed")


if __name__ == "__main__":
    main()
