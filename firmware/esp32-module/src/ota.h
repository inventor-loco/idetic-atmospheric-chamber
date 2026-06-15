// ota.h — over-the-air firmware update via HTTP pull (PROJECT_SEED §5 extension).
//
// Flow: the Pi publishes chamber/m{N}/cmd/ota with a URL to a firmware .bin it
// serves, the expected MD5, and the size. This module streams the image into
// the inactive OTA partition, verifies the MD5, and reboots into it.
//
// SAFETY: OTA is destructive and ends in a reboot. The caller MUST guarantee
// the heater is OFF and the module is in a safe state before calling run().
// On reboot the gate pulldown holds the heater off until firmware re-arms.
#pragma once
#include <Arduino.h>
#include <functional>

class Ota {
 public:
  // phase is one of: STARTED, DOWNLOADING, SUCCESS, FAILED.
  // progress is 0..100 (meaningful for DOWNLOADING), err is "" unless FAILED.
  using StatusCb = std::function<void(const char* phase, int progress, const char* err)>;

  void begin(StatusCb cb) { status_ = std::move(cb); }

  // Streams + applies the image. On success this calls ESP.restart() and does
  // not return. On failure it returns false and the caller stays on the
  // current firmware. `expected_size` may be 0 (use Content-Length instead).
  bool run(const String& url, const String& expected_md5, size_t expected_size);

 private:
  void report_(const char* phase, int progress, const char* err);
  StatusCb status_;
};
