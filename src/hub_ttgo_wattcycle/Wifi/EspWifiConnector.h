#pragma once

#include "Wifi/IWifiConnector.h"

namespace wattcycle::wifi {

class EspWifiConnector : public IWifiConnector {
 public:
  bool connect(const char* ssid, const char* password, uint32_t timeoutMs) override;
  bool isConnected() const override;
  void copyIpAddress(char* buffer, size_t capacity) const override;
  void loop() override;
};

}  // namespace wattcycle::wifi
