#!/usr/bin/env python3
"""Cuenta pérdidas de un latido MQTT con número de secuencia.

Mira dos cosas distintas:

- hueco de ``seq``: enteros que faltan entre el menor y el mayor recibidos
- hueco de tiempo: llegadas separadas por más de 1,5 periodos

    python3 medir_perdidas.py --dry-run
    python3 medir_perdidas.py --broker 127.0.0.1 --duration 70 --period 1
"""

from __future__ import annotations

import argparse
import itertools
import json
import sys


def analyze(samples: list[tuple[int, float]], period_s: float) -> dict[str, float]:
    if period_s <= 0:
        raise ValueError("el periodo tiene que ser > 0")
    if not samples:
        return {
            "received": 0,
            "expected": 0,
            "missing": 0,
            "loss": 0.0,
            "max_gap_s": 0.0,
            "time_dropouts": 0,
        }
    seqs = [seq for seq, _ in samples]
    times = [t for _, t in samples]
    first, last = min(seqs), max(seqs)
    expected = last - first + 1
    missing = expected - len(set(seqs))
    gaps = [b - a for a, b in itertools.pairwise(times)]
    max_gap = max(gaps) if gaps else 0.0
    time_dropouts = sum(1 for gap in gaps if gap > 1.5 * period_s)
    return {
        "received": float(len(samples)),
        "expected": float(expected),
        "missing": float(missing),
        "loss": (missing / expected) if expected else 0.0,
        "max_gap_s": max_gap,
        "time_dropouts": float(time_dropouts),
    }


def format_report(stats: dict[str, float]) -> str:
    return (
        f"recibidos={int(stats['received'])} esperados={int(stats['expected'])} "
        f"hueco_seq={int(stats['missing'])} fraccion={stats['loss']:.3f} "
        f"hueco_tiempo_max_s={stats['max_gap_s']:.3f} "
        f"saltos_de_tiempo={int(stats['time_dropouts'])}"
    )


def sample_outage() -> list[tuple[int, float]]:
    # 0..9 sin 4 ni 5. Las llegadas saltan de t=3 a t=6.
    return [
        (0, 0.0),
        (1, 1.0),
        (2, 2.0),
        (3, 3.0),
        (6, 6.0),
        (7, 7.0),
        (8, 8.0),
        (9, 9.0),
    ]


def run_dry() -> None:
    stats = analyze(sample_outage(), period_s=1.0)
    if int(stats["expected"]) != 10 or int(stats["missing"]) != 2:
        raise SystemExit(f"hueco de seq inesperado: {stats}")
    if abs(stats["loss"] - 0.2) > 1e-9:
        raise SystemExit(f"fraccion inesperada: {stats['loss']}")
    if abs(stats["max_gap_s"] - 3.0) > 1e-9 or int(stats["time_dropouts"]) != 1:
        raise SystemExit(f"hueco de tiempo inesperado: {stats}")
    empty = analyze([], period_s=1.0)
    if int(empty["received"]) != 0:
        raise SystemExit("el caso vacio no es cero")
    try:
        analyze([(1, 0.0)], period_s=0)
    except ValueError:
        pass
    else:
        raise SystemExit("un periodo nulo debería rechazarse")
    print("dry-run ok")
    print(format_report(stats))
    print("Ejemplo sintético: faltan seq 4 y 5, un salto de 3 s.")


def run_live(
    broker: str, port: int, topic: str, duration: float, period_s: float
) -> None:
    import time

    import paho.mqtt.client as mqtt

    samples: list[tuple[int, float]] = []
    started = time.monotonic()

    def on_connect(client, userdata, flags, reason_code, properties):
        del userdata, flags, properties
        failed = getattr(reason_code, "is_failure", None)
        if failed is None:
            failed = int(reason_code) != 0
        if failed:
            print(f"MQTT rechazó la conexión: {reason_code}", file=sys.stderr)
            return
        client.subscribe(topic)

    def on_message(client, userdata, message):
        del client, userdata
        try:
            payload = json.loads(message.payload.decode("utf-8", "replace"))
            seq = int(payload["seq"])
        except (json.JSONDecodeError, KeyError, TypeError, ValueError) as exc:
            print(f"mensaje sin seq: {exc}", file=sys.stderr)
            return
        samples.append((seq, time.monotonic() - started))

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    try:
        client.connect(broker, port, keepalive=60)
    except OSError as exc:
        raise SystemExit(f"no se pudo abrir {broker}:{port}: {exc}") from exc
    client.loop_start()
    try:
        time.sleep(duration)
    finally:
        client.loop_stop()
        client.disconnect()
    stats = analyze(samples, period_s)
    print(format_report(stats))
    if not samples:
        print("sin latidos: ¿topic, grupo y nodo en marcha?", file=sys.stderr)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--broker", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--topic", default="lab/51535/+/heartbeat")
    parser.add_argument("--duration", type=float, default=60.0)
    parser.add_argument(
        "--period", type=float, default=1.0, help="periodo esperado, en segundos"
    )
    args = parser.parse_args(argv)
    if args.dry_run:
        run_dry()
        return 0
    run_live(args.broker, args.port, args.topic, args.duration, args.period)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
