#include "fan.h"
#include "board_config.h"
#include <Arduino.h>

// Active-high MOSFET gate. If swapping to the TTL-RELAY04 board, invert here
// (and only here) — the rest of the firmware just calls set(true/false).
void Fan::begin() {
  pinMode(pins::FAN_SWITCH, OUTPUT);
  set(false);
}

void Fan::set(bool on) {
  on_ = on;
  digitalWrite(pins::FAN_SWITCH, on ? HIGH : LOW);
}
