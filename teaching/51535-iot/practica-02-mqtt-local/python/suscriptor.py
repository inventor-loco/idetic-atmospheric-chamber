#!/usr/bin/env python3
"""Imprime la telemetría de la práctica 2 y comprueba las claves.

Sin broker:

    python3 suscriptor.py --dry-run

En vivo (hace falta paho-mqtt):

    python3 suscriptor.py --broker 127.0.0.1 --duration 20
"""

from __future__ import annotations

import argparse
import json
import sys

CO2_KEYS = ("sensor_id", "tipo", "valor")
STATUS_KEYS = ("state", "pwm", "fan", "uptime_s", "rssi")


def topic_matches(topic_filter: str, topic: str) -> bool:
    """Filtro MQTT con + y #. Suficiente para los topics de estas prácticas."""
    pattern = topic_filter.split("/")
    parts = topic.split("/")
    for index, token in enumerate(pattern):
        if token == "#":
            return True
        if index >= len(parts):
            return False
        if token != "+" and token != parts[index]:
            return False
    return len(pattern) == len(parts)


def missing_keys(payload: dict, required: tuple[str, ...]) -> list[str]:
    return [key for key in required if key not in payload]


def check_payload(topic: str, payload: dict) -> list[str]:
    if topic.endswith("/co2"):
        return missing_keys(payload, CO2_KEYS)
    if topic.endswith("/status"):
        return missing_keys(payload, STATUS_KEYS)
    return []


def run_dry() -> None:
    if not topic_matches("lab/51535/+/co2", "lab/51535/g01/co2"):
        raise SystemExit("el filtro no acepta lab/51535/g01/co2")
    if topic_matches("lab/51535/+/co2", "chamber/m1/temps"):
        raise SystemExit("el filtro no debe aceptar topics de la cámara")
    if not topic_matches("lab/51535/#", "lab/51535/g01/cmd/led"):
        raise SystemExit("el filtro # no acepta el comando del LED")

    co2 = {"sensor_id": "g01", "tipo": "CO2", "valor": 450.0, "adc": 1200, "ts": 12}
    status = {"state": "RUN", "pwm": 0.0, "fan": False, "uptime_s": 12, "rssi": -55}
    if check_payload("lab/51535/g01/co2", co2):
        raise SystemExit("el ejemplo de co2 no cumple las claves")
    if check_payload("lab/51535/g01/status", status):
        raise SystemExit("el ejemplo de status no cumple las claves")
    incomplete = {"valor": 1}
    missing = check_payload("lab/51535/g01/co2", incomplete)
    if missing != ["sensor_id", "tipo"]:
        raise SystemExit(f"claves que faltan inesperadas: {missing}")
    print("dry-run ok")
    print("co2 de ejemplo:", json.dumps(co2, ensure_ascii=False))
    print("status de ejemplo:", json.dumps(status, ensure_ascii=False))


def run_live(broker: str, port: int, topic: str, duration: float) -> None:
    import time

    import paho.mqtt.client as mqtt

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
        text = message.payload.decode("utf-8", "replace")
        try:
            payload = json.loads(text)
        except json.JSONDecodeError:
            print(f"{message.topic} JSON ilegible: {text}")
            return
        if not isinstance(payload, dict):
            print(f"{message.topic} no es un objeto: {text}")
            return
        missing = check_payload(message.topic, payload)
        mark = "" if not missing else f"  faltan {missing}"
        print(f"{message.topic} {json.dumps(payload, ensure_ascii=False)}{mark}")

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


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--broker", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--topic", default="lab/51535/#")
    parser.add_argument("--duration", type=float, default=30.0)
    args = parser.parse_args(argv)
    if args.dry_run:
        run_dry()
        return 0
    run_live(args.broker, args.port, args.topic, args.duration)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
