# Práctica 4 — Fiabilidad: reconexión, SAFE_OFF y pérdidas

Cuarta práctica de 51535. El nodo ya publica (prácticas
[2](../practica-02-mqtt-local/README.md) y
[3](../practica-03-ds18b20/README.md)). Ahora tiene que sobrevivir a un
broker que desaparece, dejar el actuador en un estado seguro y dejar medido
cuántos mensajes se pierden.

En la cámara el actuador es el calentador. Aquí es el LED del GPIO 2. No
conectéis la fuente del hilo de NiChrome ni el MOSFET. El corte de verdad
está en `heater.cutoff()` y en `evaluateSafety()`
([`firmware/esp32-module/src/safety.cpp`](../../../firmware/esp32-module/src/safety.cpp));
esta práctica reproduce la regla escrita, no el banco de potencia.

## Objetivos

- Reconectar WiFi y MQTT sin bloquear el bucle.
- Si el enlace lleva más de 30 s caído y el actuador estaba armado, pasar a
  `SAFE_OFF`, apagar el LED y no rearmarlo solo.
- Obedecer `cmd/stop` del grupo y `lab/51535/all/cmd/stop`.
- Medir pérdidas por hueco de secuencia y por hueco de tiempo.

## Material

La placa, el LED de a bordo y el broker de la práctica 2. El sensor de gas
en el GPIO 34 es opcional: el latido publica la cuenta del ADC para que el
registro no sea solo un contador.

## Marco breve

La cámara corta el calentador si el MQTT lleva más de 30 s perdido
(`MQTT_TIMEOUT_MS` en `board_config.h`, texto en `PROJECT_SEED.md` §5).
También corta por `cmd/stop` y por el broadcast `chamber/all/cmd/stop`.
El estado `SAFE_OFF` se queda enclavado: volver el broker no rearma el
calentador; hace falta un comando nuevo.

El código de partida hace lo que hacía el enunciado antiguo: un
`while (!mqtt.connected())` con `delay`. Mientras tanto el bucle no corre,
`seq` no avanza y el LED se queda como estaba. Ese es el fallo que hay que
quitar.

Al salir de un bloqueo largo, si publicáis de golpe los `seq` atrasados, el
suscriptor ve un ráfaga y ningún hueco de secuencia. Los periodos en los que
el enlace estaba caído se cuentan y no se reenvían.

`connect()` de PubSubClient sigue siendo síncrono durante el timeout del
socket. La solución lo baja a 2 s (`setSocketTimeout`) para que el plazo de
30 s no se lo coma una espera de 15 s. Aun así, el LED puede apagarse un
par de segundos después del 30.

Lectura, no es tarea de código: `MqttClient::msSinceLastBrokerMsg()` en la
cámara no mide «desconectado». Se refresca al conectar y cuando entra un
mensaje. Un módulo conectado que no reciba comandos envejecería ese
contador igual. La práctica implementa la regla del esquema (enlace caído).
El informe puede señalar la diferencia en un párrafo; no hace falta parchear
`safety.cpp`.

## Cableado

Ninguno nuevo. GPIO 2, activo a nivel alto, como el LED de estado de la
cámara. Al arrancar queda apagado: el actuador no se habilita hasta
`cmd/run`, igual que el calentador no arranca hasta el primer setpoint.

## Preparación

`include/secrets.h` como en las prácticas anteriores.

```bash
cd teaching/51535-iot/practica-04-fiabilidad/firmware
pio run -e esp32dev_38p --target upload
pio device monitor -b 115200
```

Topics:

| Topic | Quién | Payload |
|---|---|---|
| `lab/51535/<grupo>/heartbeat` | ESP32, 1 Hz, QoS 0 | `{"seq","adc","ts","rssi","state"}` |
| `lab/51535/<grupo>/status` | ESP32, retenido | `state`, `pwm`, `fan`, `uptime_s`, `rssi` |
| `lab/51535/<grupo>/fault` | ESP32, al entrar en `SAFE_OFF` | `code`, `value`, `ts` |
| `lab/51535/<grupo>/cmd/run` | vosotros, QoS 1 | `{}` |
| `lab/51535/<grupo>/cmd/stop` | vosotros, QoS 1 | `{}` |
| `lab/51535/all/cmd/stop` | vosotros, QoS 1 | `{}` |

`pwm` es 1 cuando el estado es `RUN` y 0 en caso contrario. No es el duty
del calentador: es el indicador del actuador de clase. `fan` queda en false.

`RUN` no es un estado de la cámara. Hace el papel de «actuador habilitado»,
que allí cubren `HEATING` y `STEADY_STATE`. `IDLE` y `SAFE_OFF` sí se llaman
igual.

## Pasos

1. Arrancad el broker y el nodo. El LED sigue apagado (`IDLE`).
2. Armad:

   ```bash
   mosquitto_pub -h 127.0.0.1 -t lab/51535/g01/cmd/run -q 1 -m '{}'
   ```

   El LED se enciende y `status` pasa a `RUN`.
3. Parada explícita:

   ```bash
   mosquitto_pub -h 127.0.0.1 -t lab/51535/g01/cmd/stop -q 1 -m '{}'
   ```

   El LED se apaga, el estado queda `SAFE_OFF` y, al estar el enlace
   vivo, sale un `fault` con `STOP_CMD`. Volved a armar con `cmd/run`.
   Repetid con `lab/51535/all/cmd/stop` (vale para todos los grupos).
4. Pérdidas con el código de partida, todavía bloqueante. En otra terminal:

   ```bash
   python3 python/medir_perdidas.py --dry-run
   python3 python/medir_perdidas.py --broker 127.0.0.1 --duration 70 --period 1
   ```

   A los ~15 s parad Mosquitto unos 15 s y volved a lanzarlo. Anotad qué
   dice el script del hueco de tiempo y del hueco de `seq`. Con el
   `while` bloqueante, `seq` se queda congelado y al reconectar puede salir
   una ráfaga: el hueco de tiempo aparece y el de secuencia, no.
5. Quitad el bloqueo. Si el estado es `RUN` y el broker lleva más de
   `LAB_MQTT_TIMEOUT_MS` (30 s) sin conexión, llamad a
   `enterSafeOff("MQTT_TIMEOUT", ...)`. El LED se apaga en la placa aunque
   en ese momento no podáis publicar. El `fault` se envía al volver el
   enlace. No paséis a `RUN` solos: hace falta otro `cmd/run`.
6. Repetid la medida con una caída de unos 15 s (tiene que haber pérdidas
   y el LED debe seguir encendido: 15 < 30) y otra de unos 40 s (pérdidas
   y `SAFE_OFF`). `seq` tiene que saltarse los periodos sin enlace, no
   rellenarlos después.

## Entregables

- Firmware con reconexión no bloqueante y `SAFE_OFF` enclavado.
- Salida del script de las dos caídas (corta y larga), con el hueco de
  `seq` y el de tiempo.
- Captura del `fault` (`STOP_CMD` y `MQTT_TIMEOUT`) y del `status` retenido
  en `SAFE_OFF`.

## Rúbrica

| Criterio | Puntos |
|---|---|
| La reconexión no bloquea el bucle con un `delay` largo | 2 |
| A los 30 s de broker caído, con el nodo en `RUN`, el LED se apaga y no se rearma solo | 3 |
| `fault` al recuperar el enlace; `cmd/stop` y el broadcast hacen lo mismo | 2 |
| Medida de pérdidas: hueco de `seq` y hueco de tiempo, en la caída corta y en la larga | 3 |

## Errores frecuentes

- Apagar el LED en cuanto `connected()` es falso. El plazo es 30 s, no 0.
  Una microcorte de WiFi no puede ser un disparo.
- Rearmar al reconectar. El estado seguro tiene que aguantar hasta un
  comando.
- Incrementar `seq` solo cuando `publish` tiene éxito y, al volver, mandar
  todos los atrasados. Eso borra la pérdida.
- Probar el timeout sin haber mandado antes `cmd/run`. En `IDLE` el LED ya
  está apagado y el plazo no dispara: no hay actuador armado que proteger.
