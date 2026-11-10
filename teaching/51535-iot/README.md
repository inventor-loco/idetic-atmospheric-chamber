# 51535 — Internet de las Cosas

Prácticas de laboratorio del Máster Green Tech, IDeTIC-ULPGC, primer semestre
2026-27. Docentes: Vicente Matus y José Alberto Rabadán.

Las diapositivas de teoría van aparte. Aquí solo está lo que se construye y
se ejecuta: firmware de ESP32, un broker Mosquitto local y scripts Python.
El código de la cámara atmosférica (`firmware/esp32-module/`, `orchestrator/`)
no se modifica y no hay que flashearlo para hacer estas prácticas. Los nodos
de clase reutilizan sus convenios (pines, topics, nombres de fallo) para que
el mismo hardware sirva para enseñar y, más adelante, para la cámara.

El enunciado de una sola sesión del que sale esta secuencia está en
[`docs/Practica_Laboratorio_IoT_MQTT.doc`](../../docs/Practica_Laboratorio_IoT_MQTT.doc).
[`tools/generate_lab.py`](../../tools/generate_lab.py) solo regenera ese
documento antiguo.

## Secuencia

Cada práctica se puede entregar sola. El firmware de partida está en
`firmware/` (compila y hace el mínimo). La referencia del profesorado está en
`solucion/` y se puede borrar de la copia que se reparte al alumnado.

| Práctica | Carpeta | Qué se construye |
|---|---|---|
| 1 | [`practica-01-puesta-en-marcha`](practica-01-puesta-en-marcha/README.md) | ESP32 en marcha, ADC del sensor de gas, WiFi, estimación de ppm |
| 2 | [`practica-02-mqtt-local`](practica-02-mqtt-local/README.md) | MQTT contra Mosquitto local, JSON y el objeto `status` de la cámara |
| 3 | [`practica-03-ds18b20`](practica-03-ds18b20/README.md) | Dos buses 1-Wire, ROM, offset en baño común, payload `temps` |
| 4 | [`practica-04-fiabilidad`](practica-04-fiabilidad/README.md) | Reconexión, `SAFE_OFF` a los 30 s, medida de pérdidas |
| 5 | [`practica-05-energia`](practica-05-energia/README.md) | Deep sleep, presupuesto de energía, CSV y gráfica |

La práctica 1 recoge la puesta en marcha y la conversión a ppm del enunciado
antiguo. La 2 recoge el MQTT y el JSON, pero en un broker local y con topics
de laboratorio. La 5 recoge el deep sleep que allí era opcional.

## Requisitos previos

- Leer C/C++ suficiente para completar una función y flashear un ESP32.
- Python 3.11 o posterior (la misma base que el orquestador).
- PlatformIO. El camino soportado es el de la cámara (`pio run`). El `.cpp`
  es compatible con Arduino IDE si se copia a un sketch y se instalan las
  mismas librerías; los enunciados no detallan esa vía.
- Haber visto en teoría, o leer el marco de cada práctica: estación WiFi,
  MQTT publicación/suscripción, 1-Wire y los modos de bajo consumo del ESP32.

No hace falta cuenta en un broker público. No hace falta el Pi de la cámara.

## Material por grupo

Sale de piezas que el repositorio ya nombra. No es el BOM completo de un
módulo de la cámara: el calentador, las fuentes Meanwell y Traco, los MOSFET
y los ventiladores no se cablean en estas prácticas.

| Pieza | Dónde está nombrada | Uso en las prácticas |
|---|---|---|
| ESP32-DEV-38P o ESP32-DEV-30P (Seeit) | `PROJECT_SEED.md` §3.1, `firmware/esp32-module` env `esp32dev_38p` | Una placa por grupo. El binario es el mismo: los GPIO usados están en las dos. |
| Sensor de gas analógico (el enunciado antiguo dice MQ-135 o equivalente) | `docs/Practica_Laboratorio_IoT_MQTT.doc` | Prácticas 1, 2 y 5. No está en el BOM de la cámara. |
| DFRobot DFR0198 (DS18B20 estanco) | `PROJECT_SEED.md` §3.1 | Sonda de control, bus GPIO 4. El pedido de la cámara es de 6 unidades en total, no una por estudiante. |
| DS18B20 desnudo | `PROJECT_SEED.md` §3.2, «still to source» | Array vertical, bus GPIO 5. En la cámara son 5 por módulo. |
| Resistencia 4,7 kΩ | `PROJECT_SEED.md` §3.2, pull-up del 1-Wire | Una por bus en la práctica 3. |
| Resistencia 10 kΩ | `PROJECT_SEED.md` §3.2, pull-down de puerta | Solo si la salida analógica del módulo de gas puede llegar a 5 V: dos unidades como divisor. |
| Protoboard, cables puente, cable USB | Enunciado antiguo | Todas. |
| Raspberry Pi 5, u otro PC con Mosquitto | `PROJECT_SEED.md` §3.1 (la cámara tiene dos Pi 5) | Broker de clase. No usar el broker del orquestador. |

El mapa de pines de la cámara está en
[`firmware/esp32-module/include/board_config.h`](../../firmware/esp32-module/include/board_config.h):
GPIO 34 (ADC, solo entrada), GPIO 2 (LED de la placa), GPIO 4 (1-Wire de
control), GPIO 5 (1-Wire del array). Estas prácticas usan esos mismos GPIO.

## Credenciales

Los `lab_config.h` llevan placeholders (`TU_SSID`, `TU_PASSWORD`,
`192.168.0.10`, `g01`) y con eso el firmware compila. Para flashear de
verdad, copia [`secrets.h.example`](secrets.h.example) a `include/secrets.h`
dentro de la práctica. Ese fichero está en `.gitignore`. No pongas la
contraseña del laboratorio en un commit.

`LAB_GROUP_ID` tiene que ser único en el aula (`g01`, `g02`, …). No uses
`m1`…`m5`.

## Broker

Configuración de clase: [`broker/mosquitto.conf`](broker/mosquitto.conf).

```bash
mosquitto -c teaching/51535-iot/broker/mosquitto.conf
```

Anónimo en el puerto 1883, igual que la fase 0 de la cámara, y solo en la
LAN del laboratorio. Los topics de las prácticas cuelgan de `lab/51535/…`,
no de `chamber/`. Aunque alguien apuntara por error al broker de la cámara,
el orquestador (suscrito a `chamber/+/+`) no los tomaría por telemetría de
un módulo. Aun así, no apuntéis los ESP32 de clase a ese broker.

## Compilar y comprobar

Firmware (cada `firmware/` y cada `solucion/`):

```bash
cd teaching/51535-iot/practica-01-puesta-en-marcha/firmware
pio run -e esp32dev_38p
```

Scripts Python, sin broker:

```bash
python3 teaching/51535-iot/practica-02-mqtt-local/python/suscriptor.py --dry-run
python3 teaching/51535-iot/practica-03-ds18b20/python/calibrar_offset.py --dry-run
python3 teaching/51535-iot/practica-04-fiabilidad/python/medir_perdidas.py --dry-run
python3 teaching/51535-iot/practica-05-energia/python/registrar_csv.py --dry-run
```

El modo en vivo de los scripts necesita `paho-mqtt` (`requirements.txt`).
`--dry-run` no lo necesita.

## Seguridad

Estas prácticas no alimentan el calentador. No conectéis la fuente
Meanwell UHP-200-24, el hilo de NiChrome ni el MOSFET de potencia.
En la práctica 4 el «actuador» es el LED de la placa (GPIO 2). En la cámara,
el corte de verdad es `heater.cutoff()` y las reglas de
[`docs/04-safety.md`](../../docs/04-safety.md): fusible térmico de 85 °C,
pull-down de puerta, y `SAFE_OFF` si se pierde el broker. Eso se lee; no se
monta en el banco de esta asignatura.

## Árbol

```
teaching/51535-iot/
├── README.md
├── requirements.txt
├── secrets.h.example
├── broker/mosquitto.conf
├── practica-01-puesta-en-marcha/
│   ├── README.md
│   ├── firmware/          ← partida, con TODO
│   └── solucion/
├── practica-02-mqtt-local/
│   ├── README.md
│   ├── firmware/
│   ├── solucion/
│   └── python/suscriptor.py
├── practica-03-ds18b20/
│   ├── README.md
│   ├── firmware/
│   ├── solucion/
│   └── python/calibrar_offset.py
├── practica-04-fiabilidad/
│   ├── README.md
│   ├── firmware/
│   ├── solucion/
│   └── python/medir_perdidas.py
└── practica-05-energia/
    ├── README.md
    ├── firmware/
    ├── solucion/
    └── python/registrar_csv.py
```

## Notas para el profesorado

No hay calendario de sesiones en este repositorio: las fechas no están
cerradas. Cada rúbrica suma 10 puntos; el peso de cada práctica en la nota
de la asignatura tampoco está cerrado.

Preguntas que este material deja abiertas:

1. Cuántos DS18B20 hay por grupo. La cámara tiene 6 DFR0198 en total y los
   DS18B20 desnudos del array siguen sin pedir (`PROJECT_SEED.md` §3.2).
   La práctica 3 funciona con una sonda por bus; el payload deja a `null`
   los huecos hasta cinco.
2. Qué módulo analógico concreto se va a repartir. El enunciado antiguo dice
   «MQ-135 o equivalente». La pendiente en ppm depende del módulo, de si la
   salida es 3,3 V o 5 V y de la resistencia de carga. La práctica 1 pide
   una recta medida por el grupo, no una curva copiada.
3. Dónde corre el Mosquitto de clase (un PC o una de las Pi 5) y que no sea
   el proceso del orquestador.
4. Si la copia para el alumnado se publica sin las carpetas `solucion/`.
5. El esquema (`docs/07-mqtt-schema.md`, `PROJECT_SEED.md` §5) dice que la
   pérdida del broker durante más de 30 s lleva a `SAFE_OFF`.
   `evaluateSafety()` en `firmware/esp32-module/src/safety.cpp` no mira si
   el cliente está desconectado: mira el tiempo desde el último mensaje
   entrante (`MqttClient::msSinceLastBrokerMsg()`, que solo se refresca al
   conectar y en el callback). Un módulo conectado que publique telemetría
   y no reciba ningún comando pasaría a `SAFE_OFF` a los 30 s. La práctica 4
   enseña la regla escrita (enlace caído), no ese comportamiento. Conviene
   decidir cuál de las dos es la que debe cumplir el firmware de la cámara.
   Este repositorio de prácticas no lo cambia.
