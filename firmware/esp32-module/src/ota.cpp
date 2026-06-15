#include "ota.h"
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClient.h>

void Ota::report_(const char* phase, int progress, const char* err) {
  if (status_) status_(phase, progress, err ? err : "");
}

bool Ota::run(const String& url, const String& expected_md5, size_t expected_size) {
  report_("STARTED", 0, "");

  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, url)) {
    report_("FAILED", 0, "http_begin");
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    report_("FAILED", 0, String("http_" + String(code)).c_str());
    return false;
  }

  int len = http.getSize();                 // -1 if server sent no length
  if (len <= 0) len = static_cast<int>(expected_size);
  if (len <= 0) {
    http.end();
    report_("FAILED", 0, "unknown_size");
    return false;
  }

  if (!Update.begin(static_cast<size_t>(len))) {
    http.end();
    report_("FAILED", 0, "no_partition_space");
    return false;
  }
  // MD5 check is enforced by Update.end() if we set the expected digest.
  if (expected_md5.length() == 32) Update.setMD5(expected_md5.c_str());

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buf[1024];
  int written = 0;
  int last_pct = -1;

  while (http.connected() && written < len) {
    size_t avail = stream->available();
    if (avail) {
      int n = stream->readBytes(buf, min(avail, sizeof(buf)));
      if (Update.write(buf, n) != static_cast<size_t>(n)) {
        Update.abort();
        http.end();
        report_("FAILED", (written * 100) / len, "write_error");
        return false;
      }
      written += n;
      int pct = (written * 100) / len;
      if (pct != last_pct && pct % 10 == 0) {   // throttle: every ~10%
        last_pct = pct;
        report_("DOWNLOADING", pct, "");
      }
    } else {
      delay(1);
    }
  }
  http.end();

  if (written != len) {
    Update.abort();
    report_("FAILED", (written * 100) / len, "short_read");
    return false;
  }

  // end(true) finalizes and verifies the MD5 if one was set.
  if (!Update.end(true) || !Update.isFinished()) {
    report_("FAILED", 100, Update.errorString());
    return false;
  }

  report_("SUCCESS", 100, "");
  delay(200);          // let the MQTT publish flush before we reboot
  ESP.restart();
  return true;         // not reached
}
