#pragma once

#include <cstdint>

namespace wattcycle::config {

struct AppConfig {
  const char* wifiSsid = "";
  const char* wifiPassword = "";
  const char* bmsBleAddress = "";
  /// XT369P bridge STA MAC (required when ESPNOW_PMK is set).
  const char* espNowBridgeMac = "";
  /// Optional 16+ char shared ESP-NOW PMK/LMK (empty = plaintext).
  const char* espNowPmk = "";
  const char* otaHostname = "wattcycle-gateway";
  uint16_t webServerPort = 6789;
  uint32_t bmsPollIntervalMs = 2000;
  uint32_t wifiConnectTimeoutMs = 30000;
  uint32_t bleConnectTimeoutMs = 15000;
};

/// Loads compile-time injected settings (from host env vars via PlatformIO).
class AppConfigFactory {
 public:
  static AppConfig fromBuildFlags();
  static bool hasWifiCredentials(const AppConfig& config);
  static bool hasBmsAddress(const AppConfig& config);
};

}  // namespace wattcycle::config
