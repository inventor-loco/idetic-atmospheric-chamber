#!/usr/bin/env python3
"""Offset de un punto para DS18B20, a partir de un baño o bloque común.

CSV (con cabecera):

    rom,rol,t_sensor_c,t_ref_c
    28:AA:BB:CC:DD:EE:FF:01,ctrl,0.20,0.00

``rol`` es opcional. Si está, vale ``ctrl`` o ``a0`` … ``a4``.
offset = media(t_ref_c - t_sensor_c). Temperatura corregida = leída + offset.

    python3 calibrar_offset.py --dry-run
    python3 calibrar_offset.py --csv medidas.csv
"""

from __future__ import annotations

import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path

ROLES = ("ctrl", "a0", "a1", "a2", "a3", "a4")

SAMPLE = """rom,rol,t_sensor_c,t_ref_c
28:00:00:00:00:00:00:01,ctrl,0.20,0.00
28:00:00:00:00:00:00:01,ctrl,0.10,0.00
28:00:00:00:00:00:00:02,a0,-0.05,0.00
28:00:00:00:00:00:00:02,a0,0.05,0.00
"""


def normalize_rom(text: str) -> str:
    raw = "".join(ch for ch in text if ch not in ":- \t").upper()
    if len(raw) != 16 or any(ch not in "0123456789ABCDEF" for ch in raw):
        raise ValueError(f"ROM no válido: {text}")
    return ":".join(raw[i : i + 2] for i in range(0, 16, 2))


def c_bytes(rom: str) -> str:
    return ", ".join(f"0x{byte}" for byte in rom.split(":"))


def read_rows(text: str) -> list[dict[str, str]]:
    reader = csv.DictReader(line for line in text.splitlines() if line.strip())
    if reader.fieldnames is None:
        raise ValueError("el CSV no tiene cabecera")
    fields = {name.strip() for name in reader.fieldnames}
    needed = {"rom", "t_sensor_c", "t_ref_c"}
    if not needed <= fields:
        raise ValueError(f"faltan columnas {sorted(needed - fields)}")
    return list(reader)


def offsets(rows: list[dict[str, str]]) -> tuple[dict[str, float], dict[str, str]]:
    deltas: dict[str, list[float]] = defaultdict(list)
    roles: dict[str, str] = {}
    for row in rows:
        rom = normalize_rom(row["rom"])
        sensor = float(row["t_sensor_c"])
        ref = float(row["t_ref_c"])
        deltas[rom].append(ref - sensor)
        role = (row.get("rol") or "").strip()
        if not role:
            continue
        if role not in ROLES:
            raise ValueError(f"rol desconocido {role!r}; usa ctrl o a0..a4")
        previous = roles.get(rom)
        if previous is not None and previous != role:
            raise ValueError(f"{rom} aparece como {previous} y como {role}")
        roles[rom] = role
    means = {rom: sum(values) / len(values) for rom, values in deltas.items()}
    return means, roles


def cpp_snippet(means: dict[str, float], roles: dict[str, str]) -> str:
    by_role = {role: rom for rom, role in roles.items()}
    lines = ["// Sustituye las tablas de include/lab_config.h."]
    if "ctrl" in by_role:
        rom = by_role["ctrl"]
        lines.append(f"constexpr uint8_t LAB_ROM_CTRL[8] = {{{c_bytes(rom)}}};")
        lines.append(f"constexpr float LAB_OFFSET_CTRL = {means[rom]:.4f}f;")
    else:
        lines.append("// Sin rol ctrl: asigna LAB_ROM_CTRL a mano.")
    roms: list[str] = []
    offs: list[str] = []
    for role in ("a0", "a1", "a2", "a3", "a4"):
        rom = by_role.get(role)
        if rom is None:
            roms.append("{0, 0, 0, 0, 0, 0, 0, 0}")
            offs.append("0.0f")
        else:
            roms.append("{" + c_bytes(rom) + "}")
            offs.append(f"{means[rom]:.4f}f")
    lines.append("constexpr uint8_t LAB_ROM_ARRAY[5][8] = {")
    for item in roms:
        lines.append(f"    {item},")
    lines.append("};")
    lines.append("constexpr float LAB_OFFSET_ARRAY[5] = {" + ", ".join(offs) + "};")
    for rom, offset in means.items():
        if rom not in roles:
            lines.append(f"// sin rol {rom} offset {offset:.4f}f")
    return "\n".join(lines)


def run_dry() -> None:
    means, roles = offsets(read_rows(SAMPLE))
    ctrl = normalize_rom("28:00:00:00:00:00:00:01")
    array0 = normalize_rom("2800000000000002")
    if abs(means[ctrl] - (-0.15)) > 1e-9:
        raise SystemExit(f"offset de control {means[ctrl]} != -0.15")
    if abs(means[array0]) > 1e-9:
        raise SystemExit(f"offset de a0 {means[array0]} != 0")
    if roles[ctrl] != "ctrl" or roles[array0] != "a0":
        raise SystemExit(f"roles inesperados: {roles}")
    try:
        normalize_rom("no-es-un-rom")
    except ValueError:
        pass
    else:
        raise SystemExit("un ROM corto debería rechazarse")
    print("dry-run ok")
    print(cpp_snippet(means, roles))
    print("Ejemplo de baño, no es una calibración vuestra.")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--csv", type=Path)
    args = parser.parse_args(argv)
    if args.dry_run:
        run_dry()
        return 0
    if args.csv is None:
        raise SystemExit("pasa --csv medidas.csv, o --dry-run")
    means, roles = offsets(read_rows(args.csv.read_text(encoding="utf-8")))
    for rom, offset in means.items():
        role = roles.get(rom, "-")
        family = rom.split(":", 1)[0]
        note = "" if family == "28" else "  aviso: el primer byte no es 28 (DS18B20)"
        print(f"{rom}  rol={role}  offset_C={offset:.4f}{note}")
    print()
    print(cpp_snippet(means, roles))
    return 0


if __name__ == "__main__":
    sys.exit(main())
