# Práctica 1 — Puesta en marcha del ESP32, sensor y WiFi

Primera práctica de 51535. Convierte la parte de puesta en marcha del
enunciado antiguo
([`docs/Practica_Laboratorio_IoT_MQTT.doc`](../../../docs/Practica_Laboratorio_IoT_MQTT.doc)):
placa, lectura del sensor de gas y WiFi. MQTT empieza en la
[práctica 2](../practica-02-mqtt-local/README.md). La conversión a ppm, que
allí era una tarea de entrega, es el trabajo de esta sesión. El deep sleep
queda para la [práctica 5](../practica-05-energia/README.md).

El código de partida está en [`firmware/`](firmware/). Compila y publica por
el puerto serie el ADC, la tensión y `ppm=-1` hasta que completes la
conversión. La referencia está en [`solucion/`](solucion/).

## Objetivos

- Dejar PlatformIO listo y flashear un ESP32-DEV-38P o un ESP32-DEV-30P.
- Leer un sensor de gas analógico por el ADC del GPIO 34.
- Unir el ESP32 a la WiFi del laboratorio e imprimir IP y RSSI.
- Sustituir la cuenta del ADC por una estimación de concentración, con una
  ecuación y unas constantes que hayáis medido.

## Material

- 1 placa ESP32-DEV-38P o ESP32-DEV-30P.
- 1 sensor de gas analógico. El enunciado antiguo dice «MQ-135 o
  equivalente»: un sensor de óxido metálico, no un NDIR de CO2. Responde a
  varios gases y a la humedad. El número que llamamos ppm en esta práctica
  es una estimación de laboratorio, no una medida de CO2 con trazabilidad.
- Protoboard, cables puente, cable USB.
- Si la salida analógica del módulo puede subir de 3,3 V: dos resistencias
  de 10 kΩ (el valor que el proyecto usa como pull-down de puerta) para un
  divisor.

## Marco breve

El ESP32-WROOM de estas placas tiene WiFi y un ADC de 12 bits (cuentas
0…4095). El GPIO 34 es solo entrada y pertenece al ADC1. El ADC2 deja de
ser fiable con la radio encendida; por eso el enunciado antiguo, y la
cámara, reservan el 34 para una lectura analógica
(`pins::THERMAL_FUSE_STAT` en `board_config.h` es el mismo GPIO).

El GPIO 2 es el LED de la placa (`pins::STATUS_LED`). Evitad GPIO 0, 1, 3
(arranque y UART), 6–11 (flash) y 12 y 15 (straps de arranque). Es la misma
lista que `PROJECT_SEED.md` §4.5.

La conversión lineal `V = cuenta * 3,3 / 4095` es una primera aproximación.
El ADC del ESP32 no es lineal. Basta para esta práctica.

## Cableado

```
ESP32-DEV-38P o 30P          módulo de gas
3V3 o 5V  ------------------ VCC     según la serigrafía del módulo
GND       ------------------ GND
GPIO 34   ------------------ AO      solo si AO se queda en 0–3,3 V
```

Antes de unir AO al GPIO, mide la salida con el polímetro del laboratorio
mientras el módulo está alimentado. Si AO puede acercarse a 5 V, no lo
conectes directo: dos resistencias de 10 kΩ en serie entre AO y GND, y el
punto medio al GPIO 34. El ESP32 no admite 5 V en un pin.

Muchos módulos MQ traen un calefactor que tiene que llevar un rato
encendido antes de que la lectura se asiente. No desconectes VCC entre
medidas.

## Preparación

Desde la raíz del repositorio:

```bash
cd teaching/51535-iot/practica-01-puesta-en-marcha/firmware
pio run -e esp32dev_38p
pio run -e esp32dev_38p --target upload
pio device monitor -b 115200
```

La WiFi no está en el código que se entrega. Copia
[`../../secrets.h.example`](../../secrets.h.example) a `firmware/include/secrets.h`
y rellena `LAB_WIFI_SSID` y `LAB_WIFI_PASSWORD`. `secrets.h` no se sube al
repositorio. Sin ese fichero el firmware compila y arranca, pero no entra
en la red: verás el aviso de tiempo agotado y seguirá imprimiendo el ADC.

## Pasos

1. Compila y flashea el código de partida. El LED (GPIO 2) parpadea tres
   veces al arrancar. Si el monitor serie no muestra nada, confirma 115200.
2. Anota `adc` y `V` con el sensor en el aire del laboratorio, durante al
   menos un minuto, cuando la lectura ya no derive.
3. Comprueba el signo. Acerca al sensor un gas al que responda (un soplo
   cambia a la vez CO2 y humedad; sirve para ver el sentido, no para
   calibrar). La cuenta debe moverse de forma repetible. Si al subir el
   gas la cuenta baja, la pendiente de vuestra recta es negativa.
4. Completa `estimarPpm()` para que deje de devolver `-1`.
5. Vuelve a flashear. La línea del monitor tiene que mostrar un ppm que
   cambie con el paso 3 y que valga vuestra referencia cuando el sensor
   está en el aire que usasteis de base.

## Tarea: de cuentas a ppm

El código de partida ya convierte cuentas a voltios. Falta la
concentración.

No copies una fórmula con coeficientes sacados de un blog. El datasheet de
un MQ-135 es una gráfica logarítmica de Rs/R0 frente a ppm para varios
gases, y el R0 depende del módulo y de la resistencia de carga. Sin ese
dato, un `pow(ratio, b)` con constantes ajenas no es una calibración.

Lo que se pide es una recta de dos puntos medida por el grupo:

```
ppm = PPM_AIRE + (adc - ADC_AIRE) * PPM_POR_CUENTA
```

- `ADC_AIRE`: la cuenta media del paso 2.
- `PPM_AIRE`: el nivel que asignáis a ese aire. Si no tenéis una referencia
  mejor, podéis usar 450 como orden de magnitud del aire exterior y decirlo
  así en el informe. No es una calibración del sensor.
- `PPM_POR_CUENTA`: la pendiente. Con un solo punto de aire limpio todavía
  no está determinada. Hace falta un segundo punto (otra mezcla, u otra
  lectura estable que justifiquéis) o una pendiente provisional claramente
  marcada como tal. El signo sale del paso 3.

La solución de referencia implementa esa recta con constantes de ejemplo
para que el programa imprima un número. Esas constantes no son las
vuestras: en la entrega tienen que salir de vuestra medida.

## Entregables

- Firmware que imprime `adc`, tensión, `ppm`, y, con WiFi, IP y RSSI.
- `secrets.h` fuera del repositorio.
- Informe de una página como máximo: problemas al flashear o al unir la
  WiFi, ecuación de ppm, constantes medidas y qué gas creéis que estáis
  viendo en realidad (pista: no es un NDIR).

## Rúbrica

| Criterio | Puntos |
|---|---|
| Compila, se flashea y el monitor muestra lecturas periódicas | 2 |
| Cableado seguro (AO dentro de 0–3,3 V) y tensión coherente con la cuenta | 2 |
| WiFi con IP y RSSI; la contraseña no está en el código entregado | 2 |
| ppm: ecuación, constantes del grupo y signo comprobado | 3 |
| Informe (≤ 1 página) con los fallos de puesta en marcha y cómo se resolvieron | 1 |

## Errores frecuentes

- Monitor serie a 9600: se ve basura. El proyecto usa 115200, como la cámara.
- AO a 5 V directo al GPIO 34: puedes dañar el ESP32.
- Esperar un ppm de CO2 de catálogo en un MQ. Si el número no se mueve al
  cambiar el aire, la conversión está aplicada a una cuenta congelada o el
  calefactor del sensor aún no ha calentado.
- `while (WiFi.status() != WL_CONNECTED)` infinito, como el enunciado
  antiguo. Aquí el intento corta a los 15 s para que podáis leer el sensor
  aunque la clave sea incorrecta. La WiFi sigue siendo obligatoria en la
  entrega.
