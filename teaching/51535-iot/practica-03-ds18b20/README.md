# Práctica 3 — Array 1-Wire DS18B20: ROM y offset

Tercera práctica de 51535. El nodo deja el sensor de gas como fuente
principal y pasa a leer temperatura como la cámara: un DS18B20 de control
en un bus y hasta cinco en otro. El payload es el de
[`docs/07-mqtt-schema.md`](../../../docs/07-mqtt-schema.md), bajo
`lab/51535/<grupo>/temps` en vez de `chamber/m{N}/temps`.

Antes: [práctica 2](../practica-02-mqtt-local/README.md). Después:
[práctica 4](../practica-04-fiabilidad/README.md).

El código de partida enumera los ROM, lee por índice y publica el JSON sin
corregir. La referencia está en [`solucion/`](solucion/). El orden por
índice no es estable: el TODO es fijar cada sonda por su ROM y sumar el
offset del baño común.

`tools/calibrate-ds18b20.py` es el hueco de la fase 3 de la cámara (recta
con offset y ganancia, todavía sin implementar). Esta práctica no lo toca.
El cálculo de un punto está en [`python/calibrar_offset.py`](python/calibrar_offset.py).

## Objetivos

- Montar dos buses 1-Wire con pull-up de 4,7 kΩ, en los GPIO de la cámara.
- Leer el identificador ROM de 64 bits de cada DS18B20.
- Corregir el offset con un baño o un bloque común, no con el orden de
  enumeración.
- Publicar `{"ctrl", "array", "ts"}` con cinco posiciones de array.

## Material

- La placa de las prácticas 1 y 2.
- Una sonda de control: DFRobot DFR0198 (DS18B20 estanco). El pedido de la
  cámara es de seis unidades en total; si no hay una por grupo, un DS18B20
  desnudo en el GPIO 4 cumple el mismo papel.
- Hasta cinco DS18B20 para el array (en la cámara son cinco por módulo; en
  el repositorio siguen «por pedir»). Con uno por bus la práctica ya se
  puede entregar: los huecos salen `null`.
- Una resistencia de 4,7 kΩ por bus (el pull-up que fija el proyecto).
- Un vaso de agua, hielo si hacéis el punto 0 °C, o un bloque de aluminio
  donde todas las sondas vean la misma temperatura. El DFR0198 se puede
  sumergir por la cápsula, no por el conector. Un DS18B20 en cápsula TO-92
  no es estanco: metedlo en una bolsa pequeña o no lo mojéis y usad el bloque.

El broker es el de la práctica 2.

## Marco breve

El DS18B20 habla 1-Wire: varios sensores comparten un cable de datos y cada
uno tiene un ROM de 64 bits (el primer byte de un DS18B20 es `28`). La
cámara usa dos buses para no mezclar la sonda de control del hilo caliente
con el array vertical de Cn² (`PROJECT_SEED.md` §4.4):

| Función | GPIO | Constante |
|---|---|---|
| Control (DFR0198) | 4 | `pins::CTRL_SENSOR_1WIRE` |
| Array, de abajo arriba | 5 | `pins::ARRAY_1WIRE` |

Alimentación a 3,3 V (el DS18B20 admite 3,0–5,5 V). Modo de tres hilos:
VDD, GND y DQ. No uséis alimentación parásita. El pull-up interno del
ESP32 se queda corto; hace falta la resistencia de 4,7 kΩ de DQ a 3,3 V.

La conversión a 12 bits dura unos 750 ms. Por eso la cámara muestrea a 1 Hz
(`CONTROL_PERIOD_MS`). Si pedís la conversión en los dos buses y esperáis
una sola vez, las conversiones se solapan. El firmware de partida lo hace
así y publica cada 2 s para dejar sitio al MQTT.

`DallasTemperature` devuelve los sensores en un orden de búsqueda que puede
cambiar al reiniciar. Indexar `getTempCByIndex(0)` como «el de abajo» es el
fallo que el propio `sensors.cpp` de la cámara deja anotado: hay que fijar
las direcciones después de calibrar.

## Cableado

```
Bus de control (GPIO 4)                 Bus del array (GPIO 5)
3V3 ----+---- VDD del DFR0198           3V3 ----+---- VDD de cada DS18B20
        |                                       |
       4,7k                                    4,7k
        |                                       |
GPIO4 --+---- DQ                          GPIO5 --+---- DQ (todos en paralelo)
GND --------- GND                          GND -------- GND
```

Un solo punto de masa con la placa. No pongáis la sonda de control en el
bus del array: en la cámara son buses distintos y el firmware de prácticas
también.

## Preparación

`firmware/include/secrets.h`, como en la práctica 2. `LAB_GROUP_ID` único.

```bash
cd teaching/51535-iot/practica-03-ds18b20/firmware
pio run -e esp32dev_38p --target upload
pio device monitor -b 115200
```

En el monitor tiene que aparecer el número de sensores de cada bus y una
línea `28:..:..` por sonda. Si el contador es 0, el pull-up o el VDD están
mal: un bus sin resistencia de pull-up suele leer cero dispositivos.

## Pasos

1. Anotad el ROM de cada sonda y la posición física (control, o array de
   abajo arriba). No atéis el informe al índice `[0]`, `[1]`, …
2. Baño o bloque común. Las sondas quietas, juntas, hasta que cada una se
   mueva menos de 0,1 °C durante un minuto.
3. Referencia:
   - Hielo picado y agua limpia en equilibrio: 0 °C. No vale un cubito
     flotando en un vaso grande.
   - Sin punto de hielo: un termómetro de laboratorio, y lo decís.
   - Sin ninguna referencia absoluta: usad la media de las sondas como
     `t_ref_c`. Eso solo quita diferencias entre ellas, que es lo que
     importa para una estructura de temperatura. No da una temperatura
     absoluta.
4. CSV con una fila por lectura (varias filas del mismo ROM):

   ```text
   rom,rol,t_sensor_c,t_ref_c
   28:AA:BB:CC:DD:EE:FF:01,ctrl,0.20,0.00
   28:11:22:33:44:55:66:02,a0,0.05,0.00
   ```

   `rol` es `ctrl` o `a0`…`a4` de abajo arriba. El offset es la media de
   `t_ref_c - t_sensor_c`. La temperatura corregida es `leída + offset`.

   ```bash
   python3 python/calibrar_offset.py --dry-run
   python3 python/calibrar_offset.py --csv medidas.csv
   ```

   El script imprime un bloque para pegar en `lab_config.h`.
5. Completad la lectura para que elija el sensor por ROM y sume ese offset.
   Si el ROM configurado no está en el bus, publicad `null` en ese hueco,
   no la sonda vecina.
6. Repetid el baño. Tras la corrección, las sondas que estaban en el mismo
   punto tienen que coincidir dentro de unas décimas de grado. Unas pocas
   milésimas es el objetivo posterior de la cámara, con mejor referencia;
   no es el criterio de esta práctica.
7. El JSON publicado en `lab/51535/<grupo>/temps`:

   ```json
   {"ctrl":24.10,"array":[23.9,24.0,null,null,null],"ts":12}
   ```

   `ts` es segundos desde el arranque, como en el firmware de la cámara.
   `array` tiene siempre cinco elementos.

El punto de ebullición, cerca del nivel del mar, anda por 100 °C y baja si
el laboratorio no está a esa presión. No hace falta para el offset de un
punto. Si queréis una ganancia (dos puntos) es optativo y no entra en la
rúbrica; es el ajuste que el script de la cámara todavía no hace.

## Entregables

- Firmware que publica `temps` con ROM fijo y offset aplicado.
- El CSV del baño y la salida del script.
- Una captura en la que, tras corregir, las sondas del mismo baño coinciden
  dentro de 0,1 °C, o una explicación medida de por qué no llegan.

## Rúbrica

| Criterio | Puntos |
|---|---|
| Dos buses con pull-up de 4,7 kΩ y ROM impresos (o un bus, si solo hay una sonda, y el otro a `null`) | 2 |
| Cada sonda identificada por ROM, no por el índice de enumeración | 3 |
| Offset de baño común, CSV y constantes en el firmware | 3 |
| Payload `{"ctrl", "array" de 5, "ts"}` en `lab/51535/<grupo>/temps` | 2 |

## Errores frecuentes

- Llamar a la constante del GPIO 4 `PIN_CTRL`. En este ESP-IDF ese nombre ya
  es un registro del SoC y el fuente no compila. En el código de partida el
  pin se llama `PIN_BUS_CTRL`.
- Olvidar el pull-up. El síntoma habitual es cero sensores.
- DQ a 5 V con el ESP32 a 3,3 V.
- Sumergir el conector del DFR0198, o un TO-92 desnudo.
- Calibrar el índice 0 y reiniciar: la búsqueda 1-Wire puede reordenar.
- Aplicar el offset al valor `-127` (`DEVICE_DISCONNECTED_C`). Eso no es una
  temperatura. Hay que publicar `null`.
