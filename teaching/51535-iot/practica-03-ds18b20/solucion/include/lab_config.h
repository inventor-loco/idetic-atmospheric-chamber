#pragma once

#include <stdint.h>

#if defined(__has_include)
#  if __has_include("secrets.h")
#    include "secrets.h"
#  endif
#endif

#ifndef LAB_WIFI_SSID
#define LAB_WIFI_SSID "TU_SSID"
#endif

#ifndef LAB_WIFI_PASSWORD
#define LAB_WIFI_PASSWORD "TU_PASSWORD"
#endif

#ifndef LAB_MQTT_HOST
#define LAB_MQTT_HOST "192.168.0.10"
#endif

#ifndef LAB_MQTT_PORT
#define LAB_MQTT_PORT 1883
#endif

#ifndef LAB_GROUP_ID
#define LAB_GROUP_ID "g01"
#endif

// ROM todo ceros = aun no asignado: la solucion cae al indice y offset 0.
// Pega aqui la salida de python/calibrar_offset.py con los ROM del grupo.
constexpr uint8_t LAB_ROM_CTRL[8] = {0, 0, 0, 0, 0, 0, 0, 0};
constexpr float LAB_OFFSET_CTRL = 0.0f;

constexpr uint8_t LAB_ROM_ARRAY[5][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
};
constexpr float LAB_OFFSET_ARRAY[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
