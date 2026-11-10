#pragma once

// Si existe include/secrets.h (copia de secrets.h.example), sus #define
// ganan a los placeholders de abajo. No subas secrets.h.

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

// Fondo de escala aproximado del ADC. No es una calibracion del convertidor.
#ifndef LAB_ADC_VREF
#define LAB_ADC_VREF 3.3f
#endif

#ifndef LAB_ADC_FULL_SCALE
#define LAB_ADC_FULL_SCALE 4095.0f
#endif
