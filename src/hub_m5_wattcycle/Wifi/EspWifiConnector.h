#pragma once

#include "Wifi/IWifiConnector.h"

namespace wattcycle::wifi {

class EspWifiConnector : public IWifiConnector {
 public:
  bool connect(const char* ssid, const char* password, uint32_t timeoutMs) override;
  bool isConnected() const override;
  void copyIpAddress(char* buffer, size_t capacity) const override;
  uint8_t resolveSsidChannel(const char* ssid) override;
  void loop() override;

 private:
  uint8_t scanAndCache(const char* ssid);

  char ssid_[33] = {};
  char password_[65] = {};
  uint8_t bssid_[6] = {};
  uint8_t cachedChannel_ = 0;
  bool haveBssid_ = false;
  bool everConnected_ = false;
  uint32_t lastRetryMs_ = 0;
};

}  // namespace wattcycle::wifi
