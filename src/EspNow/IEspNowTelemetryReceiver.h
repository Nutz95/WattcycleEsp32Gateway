#pragma once

#include <cstdint>

namespace wattcycle::espnow_rx {

class IEspNowTelemetryReceiver {
 public:
  virtual ~IEspNowTelemetryReceiver() = default;

  /// Register ESP-NOW receive callback (Wi-Fi must already be up).
  /// When pmk is 16+ chars, bridgeMac must be the XT369P STA MAC (encrypted peer).
  virtual bool begin(const char* bridgeMac = "", const char* pmk = "") = 0;
  virtual void loop() = 0;
  virtual bool isReady() const = 0;
  /// Send meter reset command to the last XT369P bridge that published telemetry.
  virtual bool sendMeterCommand(uint8_t command) = 0;
};

}  // namespace wattcycle::espnow_rx
