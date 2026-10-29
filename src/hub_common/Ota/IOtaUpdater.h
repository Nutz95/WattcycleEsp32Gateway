#pragma once

namespace wattcycle::ota {

/// ArduinoOTA-style updater bound to a hostname.
class IOtaUpdater {
 public:
  virtual ~IOtaUpdater() = default;

  /// Start listening for OTA under `hostname`.
  virtual bool begin(const char* hostname) = 0;
  /// Pump the OTA service once.
  virtual void loop() = 0;
};

}  // namespace wattcycle::ota
