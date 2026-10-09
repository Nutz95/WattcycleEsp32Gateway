#pragma once

#include "EspNow/IEspNowCommandSender.h"

#include <cstdint>

namespace wattcycle::espnow_rx {

class IEspNowTelemetryReceiver : public IEspNowCommandSender {
 public:
  ~IEspNowTelemetryReceiver() override = default;

  /// STA mode + matching 2.4 GHz channel required (IP optional).
  /// channel 0 = use current Wi-Fi channel; 1-13 = force via esp_wifi_set_channel.
  virtual bool begin(const char* bmsBridgeMac = "", const char* xtBridgeMac = "",
                     const char* ecoflowBridgeMac = "", const char* pmk = "",
                     uint8_t channel = 0) = 0;
  virtual void loop() = 0;
  virtual bool isReady() const = 0;
};

}  // namespace wattcycle::espnow_rx
