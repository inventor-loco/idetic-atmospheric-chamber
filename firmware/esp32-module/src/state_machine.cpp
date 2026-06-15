#include "state_machine.h"

const char* stateName(State s) {
  switch (s) {
    case State::IDLE:         return "IDLE";
    case State::HEATING:      return "HEATING";
    case State::STEADY_STATE: return "STEADY_STATE";
    case State::PURGING:      return "PURGING";
    case State::SETTLING:     return "SETTLING";
    case State::READY:        return "READY";
    case State::SAFE_OFF:     return "SAFE_OFF";
    case State::FAULT:        return "FAULT";
  }
  return "UNKNOWN";
}
