#pragma once

namespace wattcycle::ota {

class IOtaUpdater {
 public:
  virtual ~IOtaUpdater() = default;

  virtual bool begin(const char* hostname) = 0;
  virtual void loop() = 0;
};

}  // namespace wattcycle::ota
