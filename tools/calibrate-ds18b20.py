#!/usr/bin/env python3
"""Calibrate DS18B20 sensors against a reference (PROJECT_SEED Phase 3).

Records each sensor's reading at known reference points (ice bath 0 °C, boiling
point adjusted for local altitude, optionally a NIST-traceable thermometer),
fits a per-sensor linear offset/gain, and writes calibration constants keyed by
the sensor's 1-Wire ROM address.

Output feeds firmware/esp32-module per-module calibration and the analysis
pipeline so the Cn² array readings agree to within a few mK.
"""

from __future__ import annotations

import argparse


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module", required=True, help="module id, e.g. m1")
    parser.add_argument("--out", default="calibration.yaml")
    parser.parse_args()
    raise SystemExit("TODO(Phase 3): implement two-point calibration capture")


if __name__ == "__main__":
    main()
