# Práctica 5 — Energía: deep sleep, presupuesto, CSV

Última práctica de 51535. Cierra el deep sleep que el enunciado antiguo
dejaba como apartado opcional, y lo convierte en un presupuesto: cuánto
cuesta cada muestra despierta y cuánto deja de costar dormir.

Antes: [práctica 4](../practica-04-fiabilidad/README.md). El sensor y la
recta de ppm son los de la [práctica 1](../practica-01-puesta-en-marcha/README.md).
El topic de publicación es otra vez `lab/51535/<grupo>/co2`, para poder
reutilizar el suscriptor de la práctica 2.

## Objetivos

- Publicar una muestra y entrar en deep sleep, en vez de esperar con `delay`.
- Volver a dormir aunque el WiFi o el broker fallen.
- Calcular la energía de un ciclo y compararla con el nodo siempre despierto.
- Registrar el topic en CSV y sacar una gráfica.

## Material

El de la práctica 2: ESP32, sensor de gas, broker local, cable USB. No hay
pinza amperimétrica en el BOM del repositorio. Si el laboratorio tiene una,
usadla. Si no, tomad la corriente del datasheet del ESP32 y dejad escrito
que es del chip, no de la placa de desarrollo.

## Marco breve

`delay(60000)` deja la CPU despierta. El deep sleep apaga casi todo el
ESP32 y lo despierta con un temporizador, por un reinicio: `setup()` vuelve
a ejecutarse y las variables normales se pierden. Lo que tiene que sobrevivir
(el número de arranque) va en memoria RTC:

```cpp
RTC_DATA_ATTR uint32_t boot_count = 0;
```

Con el `delay` del código de partida el número no avanza solo: hace falta
pulsar reset (la memoria RTC sobrevive a ese reset, así que no contéis un
reset manual como prueba). Cuando el deep sleep es real, `boot_count` crece
una vez por muestra sin tocar la placa, y el monitor serie se cae y vuelve
entre medias. Esa es la prueba; no basta con que el JSON lleve `"sleep_s": 60`.

Antes de dormir hay que soltar la radio (`WiFi.disconnect` y
`WiFi.mode(WIFI_OFF)`). Si no, algunas placas reinician por el pico de
corriente o no llegan a dormir.

El código de partida espera 60 s despierto, que es el periodo del enunciado
antiguo. La práctica 2 publicaba cada 5 s, también despierta. El presupuesto
compara esos dos hábitos con las corrientes que justifiquéis:

```
E_ciclo   = V · (I_despierto · t_despierto + I_sueño · t_sueño)
E_siempre = V · I_despierto · T
```

`T` es el periodo (60 s en el enunciado). `t_sueño = T - t_despierto`, con
`t_despierto` medido: el campo `awake_ms` del JSON. No uséis una corriente
de deep sleep del chip (microamperios) para una placa que sigue alimentando
el USB-UART: en la placa de desarrollo el sueño se queda en miliamperios.
Decid cuál de las dos cifras estáis usando.

Los números por defecto del script (`--i-awake-ma 150`, `--i-sleep-ma 10`,
`--voltage 5`) son un ejemplo para que `--dry-run` tenga una cuenta
comprobable. No son una medida de vuestra placa ni del ESP32-DEV-38P.

## Cableado

El de la práctica 1. El deep sleep apaga el LED; no indica que el nodo siga
vivo. El indicador es el mensaje MQTT y la línea de arranque del monitor
serie. El monitor se corta en cada sueño: es normal, la CPU se reinicia.

## Preparación

```bash
cd teaching/51535-iot/practica-05-energia/firmware
pio run -e esp32dev_38p --target upload
pio device monitor -b 115200
```

`include/secrets.h` como en el resto. `LAB_SLEEP_S` está en `lab_config.h`
(60). Podéis bajarlo a 20 mientras depuráis y dejarlo en 60 para la entrega.

Comprobación del script, sin placa:

```bash
python3 python/registrar_csv.py --dry-run
python3 python/registrar_csv.py --dry-run --out /tmp/p5.csv --plot /tmp/p5.svg
```

En vivo:

```bash
python3 -m pip install -r ../../requirements.txt
python3 python/registrar_csv.py --broker 127.0.0.1 --duration 180 \
    --out medidas.csv --plot medidas.svg \
    --i-awake-ma TU_MEDIDA --i-sleep-ma TU_MEDIDA --voltage 5 --period 60
```

`medidas.csv` no se sube al repositorio: `*.csv` está en el `.gitignore`
del proyecto. Entregad el CSV por el canal que indique el profesorado, o
pegad en el informe las primeras líneas y la cifra de energía.

## Pasos

1. Flashead el código de partida. Tiene que publicar cada minuto, con
   `awake_ms` pequeño (el tiempo de conectar y publicar). `arranque` no
   sube entre publicaciones: el minuto de espera es un `delay`.
2. Sustituid el `delay` por deep sleep del mismo periodo. Al despertar, el
   monitor tiene que imprimir `arranque=2`, luego 3, y así. `seq` del JSON
   sigue ese contador.
3. Desconectad el WiFi del laboratorio (o poned una clave falsa) y
   comprobad que la placa vuelve a dormir. `fallos` en memoria RTC tiene
   que subir. Un nodo que se queda reintentando la WiFi se come la ventaja
   del sueño.
4. Con el enlace bien, dejad el suscriptor el tiempo de varias muestras.
   La gráfica SVG sale de `valor` (el ppm). Si aún no habéis cerrado la
   recta de la práctica 1, el código de partida grafica la cuenta del ADC;
   la entrega quiere el ppm.
5. Calculad `E_ciclo` y `E_siempre` con vuestras corrientes. El script lo
   imprime al cerrar la ventana. En el informe: de dónde sale cada
   corriente, si es del chip o de la placa, y cuántas veces baja la energía
   por muestra al dormir. No hace falta un porcentaje de catálogo.

`awake_ms` se toma justo antes de publicar. Los ~100 ms que el firmware de
la solución espera para que el paquete salga no entran en esa cifra. Es
pequeño al lado de un WiFi que tarda segundos; podéis sumarlo si lo medís
en el monitor.

## Entregables

- Firmware que duerme de verdad (`boot_count` creciente) y duerme también
  si no hay broker.
- CSV de al menos tres muestras y la SVG.
- Presupuesto con la fórmula, las corrientes justificadas y la comparación
  con el nodo siempre despierto (el hábito de la práctica 2, o este mismo
  periodo sin dormir).

## Rúbrica

| Criterio | Puntos |
|---|---|
| Deep sleep real: `boot_count` sube y el periodo no es un `delay` | 3 |
| Si fallan WiFi o MQTT, el nodo vuelve a dormir y lo deja contado | 2 |
| Presupuesto con corrientes justificadas, distinguiendo chip y placa de desarrollo | 3 |
| CSV y gráfica a partir de una captura, con `valor` en ppm | 2 |

## Errores frecuentes

- `ESP.deepSleep` o `esp_deep_sleep_start` sin apagar la radio, y la placa
  reinicia al momento o no despierta a la hora.
- Guardar el contador en una variable normal. Cada sueño la pone a cero y
  parece que no funciona.
- Usar 10 µA de un datasheet del chip como `I_sueño` de la DevKit con el
  cable USB puesto.
- Dejar el `while` de reconexión de la práctica 2: si no hay red, no se
  llega nunca al `sleep`.
