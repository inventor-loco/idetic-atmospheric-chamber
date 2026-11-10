#!/usr/bin/env python3
"""Guarda la telemetría de la práctica 5 en CSV y dibuja una SVG.

La energía usa las corrientes que pases. Los valores por defecto son un
ejemplo para el dry-run, no una medida de la placa ni del chip.

    python3 registrar_csv.py --dry-run
    python3 registrar_csv.py --dry-run --out /tmp/p5.csv --plot /tmp/p5.svg
    python3 registrar_csv.py --broker 127.0.0.1 --duration 180 --out medidas.csv --plot medidas.svg
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from datetime import UTC, datetime
from pathlib import Path

# Ejemplo deliberado. 150 mA despierto y 10 mA en sueño describen el orden
# de una placa de desarrollo con USB, no el deep sleep del chip a solas.
EXAMPLE_AWAKE_A = 0.15
EXAMPLE_SLEEP_A = 0.010
EXAMPLE_VOLTAGE_V = 5.0
EXAMPLE_PERIOD_S = 60.0
EXAMPLE_AWAKE_S = 2.0

COLUMNS = (
    "iso_time",
    "topic",
    "seq",
    "sensor_id",
    "tipo",
    "valor",
    "adc",
    "ctrl_c",
    "state",
    "awake_ms",
    "sleep_s",
    "rssi",
)


def energy_j(
    awake_s: float, sleep_s: float, i_awake_a: float, i_sleep_a: float, voltage_v: float
) -> float:
    return voltage_v * (i_awake_a * awake_s + i_sleep_a * sleep_s)


def budget(
    awake_s: float,
    period_s: float,
    i_awake_a: float,
    i_sleep_a: float,
    voltage_v: float,
) -> tuple[float, float, float]:
    sleep_s = max(0.0, period_s - awake_s)
    cycle = energy_j(awake_s, sleep_s, i_awake_a, i_sleep_a, voltage_v)
    always = energy_j(period_s, 0.0, i_awake_a, i_sleep_a, voltage_v)
    return cycle, always, sleep_s


def flatten(topic: str, payload: str, now: str) -> dict[str, object]:
    data = json.loads(payload)
    if not isinstance(data, dict):
        raise TypeError("el payload no es un objeto JSON")
    row: dict[str, object] = {key: "" for key in COLUMNS}
    row["iso_time"] = now
    row["topic"] = topic
    for key in (
        "seq",
        "sensor_id",
        "tipo",
        "valor",
        "adc",
        "state",
        "awake_ms",
        "sleep_s",
        "rssi",
    ):
        if data.get(key) is not None:
            row[key] = data[key]
    if data.get("ctrl") is not None:
        row["ctrl_c"] = data["ctrl"]
    return row


def write_csv(rows: list[dict[str, object]], dest: Path | None) -> None:
    handle = (
        sys.stdout if dest is None else dest.open("w", newline="", encoding="utf-8")
    )
    try:
        writer = csv.DictWriter(handle, fieldnames=COLUMNS)
        writer.writeheader()
        writer.writerows(rows)
    finally:
        if dest is not None:
            handle.close()


def series_from_rows(rows: list[dict[str, object]]) -> list[tuple[float, float]]:
    points: list[tuple[float, float]] = []
    for index, row in enumerate(rows):
        raw = row.get("valor")
        if raw == "" or raw is None:
            raw = row.get("ctrl_c")
        if raw == "" or raw is None:
            continue
        points.append((float(index), float(raw)))
    return points


def write_svg(path: Path, points: list[tuple[float, float]], ylabel: str) -> None:
    if len(points) < 1:
        raise ValueError("no hay puntos para la gráfica")
    width, height = 800, 420
    left, right, top, bottom = 70, 20, 36, 40
    plot_w = width - left - right
    plot_h = height - top - bottom
    xs = [x for x, _ in points]
    ys = [y for _, y in points]
    xmin, xmax = min(xs), max(xs)
    ymin, ymax = min(ys), max(ys)
    if xmax == xmin:
        xmax = xmin + 1.0
    if ymax == ymin:
        ymax = ymin + 1.0
        ymin -= 1.0

    def sx(x: float) -> float:
        return left + (x - xmin) / (xmax - xmin) * plot_w

    def sy(y: float) -> float:
        return top + (ymax - y) / (ymax - ymin) * plot_h

    poly = " ".join(f"{sx(x):.1f},{sy(y):.1f}" for x, y in points)
    safe = ylabel.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    svg = f"""<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
  <rect width="100%" height="100%" fill="white"/>
  <text x="{left}" y="22" font-family="sans-serif" font-size="14">{safe}</text>
  <line x1="{left}" y1="{top}" x2="{left}" y2="{top + plot_h}" stroke="black"/>
  <line x1="{left}" y1="{top + plot_h}" x2="{left + plot_w}" y2="{top + plot_h}" stroke="black"/>
  <polyline fill="none" stroke="#1f4e79" stroke-width="2" points="{poly}"/>
  <text x="{left}" y="{height - 8}" font-family="sans-serif" font-size="12">muestra</text>
  <text x="8" y="{top + 14}" font-family="sans-serif" font-size="12">{ymin:.2f} … {ymax:.2f}</text>
</svg>
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(svg, encoding="utf-8")


def sample_messages() -> list[tuple[str, str]]:
    topic = "lab/51535/g01/co2"
    payloads = [
        {
            "sensor_id": "g01",
            "tipo": "CO2",
            "valor": 460.0,
            "adc": 1220,
            "seq": 1,
            "awake_ms": 2000,
            "sleep_s": 60,
            "rssi": -58,
        },
        {
            "sensor_id": "g01",
            "tipo": "CO2",
            "valor": 470.5,
            "adc": 1240,
            "seq": 2,
            "awake_ms": 2100,
            "sleep_s": 60,
            "rssi": -60,
        },
        {
            "sensor_id": "g01",
            "tipo": "CO2",
            "valor": 455.0,
            "adc": 1210,
            "seq": 3,
            "awake_ms": 1900,
            "sleep_s": 60,
            "rssi": -57,
        },
    ]
    return [(topic, json.dumps(item)) for item in payloads]


def run_dry(out: Path | None, plot: Path | None) -> None:
    now = datetime.now(UTC).isoformat()
    rows = [flatten(topic, payload, now) for topic, payload in sample_messages()]
    if rows[0]["valor"] != 460.0:
        raise SystemExit("el CSV de ejemplo no conserva valor")
    points = series_from_rows(rows)
    if len(points) != 3:
        raise SystemExit("la serie de ejemplo no tiene 3 puntos")
    if out is not None:
        write_csv(rows, out)
    if plot is not None:
        write_svg(plot, points, "valor (ejemplo, no es una medida)")
    cycle, always, sleep_s = budget(
        EXAMPLE_AWAKE_S,
        EXAMPLE_PERIOD_S,
        EXAMPLE_AWAKE_A,
        EXAMPLE_SLEEP_A,
        EXAMPLE_VOLTAGE_V,
    )
    if abs(cycle - 4.4) > 1e-9 or abs(always - 45.0) > 1e-9:
        raise SystemExit(f"presupuesto de ejemplo incorrecto: {cycle} J, {always} J")
    if not cycle < always:
        raise SystemExit(
            "el ciclo con sueño debería quedar por debajo del siempre activo"
        )
    print("dry-run ok")
    print(
        "ejemplo (no es una medida): "
        f"V={EXAMPLE_VOLTAGE_V:.0f} V, "
        f"I_despierto={EXAMPLE_AWAKE_A * 1000:.0f} mA, "
        f"I_sueno={EXAMPLE_SLEEP_A * 1000:.0f} mA, "
        f"t_despierto={EXAMPLE_AWAKE_S:.0f} s, t_sueno={sleep_s:.0f} s"
    )
    print(
        f"energia por ciclo = {cycle:.3f} J; siempre activo en el mismo periodo = {always:.3f} J"
    )
    print(
        "Sustituye las corrientes por una medida o por el datasheet, y no mezcles chip y placa."
    )


def mean_awake_s(rows: list[dict[str, object]]) -> float:
    values = [float(row["awake_ms"]) / 1000.0 for row in rows if row["awake_ms"] != ""]
    if not values:
        return 0.0
    return sum(values) / len(values)


def run_live(args: argparse.Namespace) -> None:
    import time

    import paho.mqtt.client as mqtt

    rows: list[dict[str, object]] = []

    def on_connect(client, userdata, flags, reason_code, properties):
        del userdata, flags, properties
        failed = getattr(reason_code, "is_failure", None)
        if failed is None:
            failed = int(reason_code) != 0
        if failed:
            print(f"MQTT rechazó la conexión: {reason_code}", file=sys.stderr)
            return
        client.subscribe(args.topic)

    def on_message(client, userdata, message):
        del client, userdata
        now = datetime.now(UTC).isoformat()
        try:
            rows.append(
                flatten(message.topic, message.payload.decode("utf-8", "replace"), now)
            )
        except (json.JSONDecodeError, TypeError, ValueError) as exc:
            print(f"mensaje ignorado: {exc}", file=sys.stderr)

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    try:
        client.connect(args.broker, args.port, keepalive=60)
    except OSError as exc:
        raise SystemExit(f"no se pudo abrir {args.broker}:{args.port}: {exc}") from exc
    client.loop_start()
    try:
        time.sleep(args.duration)
    finally:
        client.loop_stop()
        client.disconnect()

    write_csv(rows, args.out)
    points = series_from_rows(rows)
    if args.plot is not None and points:
        write_svg(args.plot, points, "valor")
    if not rows:
        print("sin mensajes en la ventana", file=sys.stderr)
        return
    awake_s = mean_awake_s(rows)
    cycle, always, sleep_s = budget(
        awake_s,
        args.period,
        args.i_awake_ma / 1000.0,
        args.i_sleep_ma / 1000.0,
        args.voltage,
    )
    print(
        f"mensajes={len(rows)} t_despierto_medio={awake_s:.3f} s t_sueno={sleep_s:.3f} s"
    )
    print(f"energia por ciclo={cycle:.4f} J; siempre activo={always:.4f} J")
    print(
        "Las corrientes son las de los argumentos, no una medida hecha por el script."
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--broker", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--topic", default="lab/51535/+/co2")
    parser.add_argument("--duration", type=float, default=30.0)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--plot", type=Path)
    parser.add_argument("--i-awake-ma", type=float, default=EXAMPLE_AWAKE_A * 1000.0)
    parser.add_argument("--i-sleep-ma", type=float, default=EXAMPLE_SLEEP_A * 1000.0)
    parser.add_argument("--voltage", type=float, default=EXAMPLE_VOLTAGE_V)
    parser.add_argument("--period", type=float, default=EXAMPLE_PERIOD_S)
    args = parser.parse_args(argv)
    if args.dry_run:
        run_dry(args.out, args.plot)
        return 0
    run_live(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
