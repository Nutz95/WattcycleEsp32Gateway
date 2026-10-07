#pragma once

#include "Ota/IOtaUpdater.h"

namespace wattcycle::ota {

class ArduinoOtaUpdater : public IOtaUpdater {
 public:
  bool begin(const char* hostname) override;
  void loop() override;

 private:
  bool started_ = false;
};

}  // namespace wattcycle::ota
