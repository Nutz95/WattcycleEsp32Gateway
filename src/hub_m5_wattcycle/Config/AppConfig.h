#pragma once

#include <cstdint>

namespace wattcycle::config {

struct AppConfig {
  const char* wifiSsid = "";
  const char* wifiPassword = "";
  /// TTGO BMS bridge STA MAC (ESP-NOW peer).
  const char* espNowBmsBridgeMac = "";
  /// TTGO XT369P bridge STA MAC (ESP-NOW peer).
  const char* espNowXtBridgeMac = "";
  /// EcoFlow S3 bridge STA MAC (ESP-NOW peer).
  const char* espNowEcoflowBridgeMac = "";
  /// Optional 16+ char shared ESP-NOW PMK/LMK (empty = plaintext).
  const char* espNowPmk = "";
  const char* otaHostname = "wattcycle-gateway";
  /// POSIX TZ for M5 clock tiles (default Europe/Paris with DST).
  const char* posixTimeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
  uint16_t webServerPort = 6789;
  /// Keep short: ESP-NOW starts after this even without IP (channel from scan).
  uint32_t wifiConnectTimeoutMs = 12000;
};

class AppConfigFactory {
 public:
  static AppConfig fromBuildFlags();
  static bool hasWifiCredentials(const AppConfig& config);
};

}  // namespace wattcycle::config
