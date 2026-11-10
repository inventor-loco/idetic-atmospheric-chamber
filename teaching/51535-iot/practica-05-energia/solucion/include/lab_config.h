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

#ifndef LAB_SLEEP_S
#define LAB_SLEEP_S 60
#endif

#ifndef LAB_ADC_VREF
#define LAB_ADC_VREF 3.3f
#endif

#ifndef LAB_ADC_FULL_SCALE
#define LAB_ADC_FULL_SCALE 4095.0f
#endif

#ifndef LAB_ADC_AIRE
#define LAB_ADC_AIRE 1200
#endif

#ifndef LAB_PPM_AIRE
#define LAB_PPM_AIRE 450.0f
#endif

#ifndef LAB_PPM_POR_CUENTA
#define LAB_PPM_POR_CUENTA 0.5f
#endif
