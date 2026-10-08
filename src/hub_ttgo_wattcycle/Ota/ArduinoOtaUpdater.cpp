#include "Ota/ArduinoOtaUpdater.h"

#ifndef UNIT_TEST

#include <ArduinoOTA.h>

namespace wattcycle::ota {

bool ArduinoOtaUpdater::begin(const char* hostname) {
  if (hostname == nullptr || hostname[0] == '\0') {
    return false;
  }

  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() {});
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onError([](ota_error_t) {});
  ArduinoOTA.begin();
  started_ = true;
  return true;
}

void ArduinoOtaUpdater::loop() {
  if (started_) {
    ArduinoOTA.handle();
  }
}

}  // namespace wattcycle::ota

#endif
