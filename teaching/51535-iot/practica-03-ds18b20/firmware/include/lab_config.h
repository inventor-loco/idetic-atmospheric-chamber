#pragma once

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
