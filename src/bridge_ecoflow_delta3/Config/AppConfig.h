#pragma once

#include <cstdint>

namespace ecoflow::config {

/// Build-flag config for BLE auth + ESP-NOW TX (secrets via env only).
struct AppConfig {
  const char* wifiSsid = "";
  const char* ecoflowBleAddress = "";
  const char* ecoflowSerial = "";
  const char* ecoflowUserId = "";
  const char* espNowPeerMac = "";
  const char* espNowPmk = "";
  uint8_t espNowChannel = 0;
};

}  // namespace ecoflow::config
