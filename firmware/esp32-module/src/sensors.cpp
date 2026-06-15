#include "sensors.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

namespace {
OneWire ctrl_wire(pins::CTRL_SENSOR_1WIRE);
OneWire array_wire(pins::ARRAY_1WIRE);
DallasTemperature ctrl_bus(&ctrl_wire);
DallasTemperature array_bus(&array_wire);
}  // namespace

void Sensors::begin() {
  ctrl_bus.begin();
  array_bus.begin();
  // Blocking conversion off; we trigger then poll on our own 1 Hz cadence.
  ctrl_bus.setWaitForConversion(true);
  array_bus.setWaitForConversion(true);
  ctrl_bus.setResolution(12);
  array_bus.setResolution(12);
}

SensorReadings Sensors::read() {
  SensorReadings r;

  ctrl_bus.requestTemperatures();
  r.ctrl_c = ctrl_bus.getTempCByIndex(0);
  r.ctrl_ok = (r.ctrl_c != DEVICE_DISCONNECTED_C);

  array_bus.requestTemperatures();
  bool all_ok = true;
  for (int i = 0; i < ARRAY_SENSOR_COUNT; ++i) {
    // NOTE: index order depends on enumeration; pin to addresses after
    // calibration (tools/calibrate-ds18b20.py) so bottom→top stays fixed.
    r.array_c[i] = array_bus.getTempCByIndex(i);
    if (r.array_c[i] == DEVICE_DISCONNECTED_C) all_ok = false;
  }
  r.array_ok = all_ok;

  if (r.ctrl_ok && r.array_ok) {
    fail_count_ = 0;
  } else {
    ++fail_count_;
  }
  return r;
}
