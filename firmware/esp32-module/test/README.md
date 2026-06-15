# Firmware unit tests

PlatformIO `test/` directory. Pure-logic units (PID math in `heater.cpp`, the
fault evaluation in `safety.cpp`, the command decoder in `mqtt_client.cpp`)
should get native host tests so they can run without hardware:

```bash
pio test -e native    # add a [env:native] target in platformio.ini
```

Hardware-in-the-loop tests (1-Wire reads, LEDC output) run on-target:

```bash
pio test -e esp32dev_38p
```

See `docs/05-firmware-architecture.md` for the test strategy.
