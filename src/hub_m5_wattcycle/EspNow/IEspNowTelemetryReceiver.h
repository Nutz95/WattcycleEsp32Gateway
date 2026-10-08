#pragma once

#include <cstdint>

namespace wattcycle::espnow_rx {

class IEspNowTelemetryReceiver {
 public:
  virtual ~IEspNowTelemetryReceiver() = default;

  /// Wi-Fi must already be up. Register encrypted peers for BMS + XT bridges.
  virtual bool begin(const char* bmsBridgeMac = "", const char* xtBridgeMac = "",
                     const char* pmk = "") = 0;
  virtual void loop() = 0;
  virtual bool isReady() const = 0;
  virtual bool sendMeterCommand(uint8_t command) = 0;
};

}  // namespace wattcycle::espnow_rx
