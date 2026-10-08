#pragma once

#include <cstdint>

namespace wattcycle::config {

struct AppConfig {
  /// Used only to discover the AP channel shared with the M5 hub (no STA join).
  const char* wifiSsid = "";
  const char* bmsBleAddress = "";
  /// M5 hub STA MAC (ESP-NOW peer).
  const char* espNowPeerMac = "";
  const char* espNowPmk = "";
  uint8_t espNowChannel = 0;
  uint32_t bmsPollIntervalMs = 2000;
  uint32_t bleConnectTimeoutMs = 15000;
};

class AppConfigFactory {
 public:
  static AppConfig fromBuildFlags();
  static bool hasBmsAddress(const AppConfig& config);
  static bool hasEspNowPeer(const AppConfig& config);
};

}  // namespace wattcycle::config
