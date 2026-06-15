// fan.h — on/off fan control (PROJECT_SEED §4.3, §11).
//
// Abstracts the "fans on/off" call so the underlying hardware (MOSFET vs
// TTL relay board) can be swapped without touching the rest of the firmware.
#pragma once

class Fan {
 public:
  void begin();           // configures the GPIO, fans OFF
  void set(bool on);
  bool isOn() const { return on_; }

 private:
  bool on_ = false;
};
