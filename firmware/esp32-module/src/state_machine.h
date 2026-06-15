// state_machine.h — per-module operating states (PROJECT_SEED §5).
#pragma once

enum class State {
  IDLE,         // booted, heater 0%, awaiting first setpoint
  HEATING,      // ramping toward setpoint
  STEADY_STATE, // PID holding setpoint
  PURGING,      // fans on, heater off, clearing the chamber
  SETTLING,     // post-purge, waiting for thermal settle
  READY,        // settled, ready to resume
  SAFE_OFF,     // heater 0%, fans on (commanded stop or MQTT loss)
  FAULT         // hardware fault (e.g. thermal fuse blown); needs manual reset
};

const char* stateName(State s);
