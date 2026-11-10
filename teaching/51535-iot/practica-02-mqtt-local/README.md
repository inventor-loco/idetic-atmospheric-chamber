# Práctica 2 — MQTT en un broker local y carga JSON

Segunda práctica de 51535. Parte de la
[práctica 1](../practica-01-puesta-en-marcha/README.md) (el ESP32 ya lee el
sensor y entra en la WiFi) y sustituye el MQTT del enunciado antiguo.

Aquel enunciado publicaba la cuenta del ADC, en texto plano, cada 5 s, en
`idetic/greentech/co2` del broker público `broker.hivemq.com`, con el
identificador de cliente fijo `ESP32Client_GreenTech`. Eso no sirve en un
aula: el topic es compartido con cualquiera, el client id choca en cuanto
hay dos placas, y los datos salen de la LAN. Aquí el broker es un Mosquitto
en un PC o en una Raspberry Pi del laboratorio, y la carga es JSON.

La siguiente es la [práctica 3](../practica-03-ds18b20/README.md) (el array
de temperatura). El deep sleep del enunciado antiguo sigue en la
[práctica 5](../practica-05-energia/README.md).

## Objetivos

- Levantar Mosquitto con [`broker/mosquitto.conf`](../../broker/mosquitto.conf).
- Publicar la lectura en JSON, con `sensor_id`, `tipo` y `valor`.
- Publicar un `status` retenido con las mismas claves que
  [`docs/07-mqtt-schema.md`](../../../docs/07-mqtt-schema.md).
- Suscribirse a un comando y encender o apagar el LED.

## Material

El de la práctica 1, más un ordenador con Mosquitto (`mosquitto` y
`mosquitto-clients`) en la misma LAN que el ESP32. Puede ser un PC o una
Raspberry Pi 5. No es la Pi que corre el orquestador de la cámara.

## Marco breve

MQTT es publicación/suscripción. El ESP32 no conoce a quien escucha: publica
en un topic y el broker reparte. La cámara hace lo mismo, con PubSubClient,
ArduinoJson y topics bajo `chamber/m{N}/…`.

Convenios que vais a copiar, y los que no:

| Cámara (`docs/07`) | Esta práctica |
|---|---|
| `chamber/m1/status` | `lab/51535/<grupo>/status` |
| Claves `state`, `pwm`, `fan`, `uptime_s`, `rssi` | Las mismas claves |
| `status` retenido, telemetría en QoS 0 | Igual |
| Comandos en QoS 1, los publica el orquestador | El comando del LED se publica con `mosquitto_pub -q 1` |
| `ts` en `temps` | Segundos desde el arranque (`millis()/1000`), que es lo que envía `main.cpp` de la cámara |

PubSubClient 2.8 publica en QoS 0. No finjáis QoS 1 en el `publish` del
ESP32: la librería no lo ofrece. El esquema pide QoS 1 en los comandos, y
quien publica esos comandos es el cliente de escritorio o, en la cámara, el
orquestador Python.

`pwm` y `fan` van en el `status` para que el objeto tenga la misma forma.
En esta práctica no hay calentador ni ventilador: `pwm` queda en 0 y `fan`
en false. El LED no es el PWM del calentador; es un actuador de clase para
comprobar la suscripción.

El prefijo `lab/51535/` es deliberado. El orquestador está suscrito a
`chamber/+/+`. No publiquéis en `chamber/`.

## Cableado

El de la práctica 1. El LED es el de la placa, GPIO 2, activo a nivel alto
como en `firmware/esp32-module/src/main.cpp`.

## Preparación

Broker, en una máquina que no sea la de la cámara:

```bash
mosquitto -c teaching/51535-iot/broker/mosquitto.conf
```

En otra terminal:

```bash
mosquitto_sub -h 127.0.0.1 -t 'lab/51535/#' -v
```

Copia [`secrets.h.example`](../../secrets.h.example) a
`firmware/include/secrets.h`. `LAB_MQTT_HOST` es la IP de esa máquina vista
desde el ESP32, no `127.0.0.1` (eso sería el propio ESP32). `LAB_GROUP_ID`
único en el aula, por ejemplo `g01`.

```bash
cd teaching/51535-iot/practica-02-mqtt-local/firmware
pio run -e esp32dev_38p --target upload
pio device monitor -b 115200
```

El código de partida ya publica `{"valor": <adc>}` cada 5 s en
`lab/51535/<grupo>/co2` y un `status` retenido. Compila. Los TODO están en
el JSON completo, en la suscripción y en el callback del LED.

## Pasos

1. Comprueba con `mosquitto_sub` que llega el JSON de partida.
2. Completa el payload de `co2` para que lleve al menos `sensor_id`, `tipo`
   y `valor`. `valor` es el ppm de la práctica 1, no la cuenta cruda.
   Añade `adc` y `ts` (segundos desde el arranque). Ejemplo:

   ```json
   {"sensor_id":"g01","tipo":"CO2","valor":450.0,"adc":1200,"ts":12}
   ```

   `tipo` puede seguir siendo `"CO2"` porque así lo pedía el enunciado
   antiguo. En el informe di que el sensor no es un NDIR.
3. Suscríbete a `lab/51535/<grupo>/cmd/led` con QoS 1 dentro de `reconnect`,
   después de un `connect` correcto.
4. En el callback, lee `{"on": true}` o `{"on": false}` y mueve el GPIO 2.
5. Desde el PC:

   ```bash
   mosquitto_pub -h 127.0.0.1 -t lab/51535/g01/cmd/led -q 1 -m '{"on":true}'
   mosquitto_pub -h 127.0.0.1 -t lab/51535/g01/cmd/led -q 1 -m '{"on":false}'
   ```

   Cambia `g01` por vuestro grupo. El LED tiene que seguir el comando.
6. Cierra el monitor serie, rearranca el ESP32 y vuelve a suscribirte solo
   a `lab/51535/g01/status`. El mensaje retenido tiene que aparecer al
   momento, sin esperar 5 s. Eso es el flag retain.

El script [`python/suscriptor.py`](python/suscriptor.py) imprime lo mismo
que `mosquitto_sub`, y comprueba las claves:

```bash
python3 python/suscriptor.py --dry-run
python3 -m pip install -r ../../requirements.txt
python3 python/suscriptor.py --broker 127.0.0.1 --duration 20
```

## Qué se ha retirado del enunciado antiguo

- Broker `broker.hivemq.com` y topic `idetic/greentech/co2`.
- Client id único `ESP32Client_GreenTech`. Aquí el id es
  `lab51535-<grupo>`. Dos placas con el mismo id se pisan la sesión.
- Payload de texto con la cuenta pelada. Pasa a ser JSON.
- El `while (!client.connected())` con `delay(5000)` sigue en el código de
  partida, a propósito. La práctica 4 lo quita: mientras está bloqueado, la
  placa no puede vigilar un plazo.

## Entregables

- Firmware que publica el JSON completo y obedece `cmd/led`.
- Captura de `mosquitto_sub` (o del script) con al menos tres mensajes y
  con el `status` retenido.
- Una frase en el informe de la práctica 1, o un párrafo aparte, que diga
  por qué el broker público no vale para el aula.

## Rúbrica

| Criterio | Puntos |
|---|---|
| Mosquitto local en marcha y suscripción demostrada en la LAN | 2 |
| JSON de `co2` con `sensor_id`, `tipo`, `valor` (ppm), `adc` y `ts` | 2 |
| Topic bajo `lab/51535/<grupo>/` y client id distinto del de otro grupo | 2 |
| `status` retenido con `state`, `pwm`, `fan`, `uptime_s`, `rssi` | 2 |
| `cmd/led` enciende y apaga el GPIO 2 | 2 |

## Errores frecuentes

- `LAB_MQTT_HOST` a `127.0.0.1` en el ESP32. Esa dirección es la propia placa.
- Cortafuegos del PC que bloquea el 1883. `mosquitto_sub` en el mismo PC
  funciona y el ESP32 no.
- Dos grupos con `g01`. Los mensajes se mezclan; si además repetís el client
  id, el broker cierra una de las dos sesiones.
- Olvidar `mqtt.loop()`. Sin eso no entra el comando del LED.
