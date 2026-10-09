#pragma once

#include <cstdint>

namespace ecoflow::config {

/// Scaffold config — EcoFlow BLE / ESP-NOW fields are reserved for later milestones.
struct AppConfig {
  const char* wifiSsid = "";
  const char* ecoflowBleAddress = "";
  const char* ecoflowSerial = "";
  const char* ecoflowUserId = "";
  const char* espNowPeerMac = "";
  const char* espNowPmk = "";
  uint8_t espNowChannel = 0;
};

class AppConfigFactory {
 public:
  static AppConfig fromBuildFlags();
};

}  // namespace ecoflow::config
