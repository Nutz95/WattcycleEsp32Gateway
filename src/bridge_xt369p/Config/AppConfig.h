#pragma once

#include <cstdint>

namespace xt369p::config {

struct AppConfig {
  /// SSID of the hub's AP — used only to discover ESP-NOW channel (no join).
  const char* wifiSsid = "";
  /// Bluetooth address "AA:BB:CC:DD:EE:FF" or empty to connect by name.
  const char* xt369pBtAddress = "";
  const char* sppDeviceName = "XT369P_SPP";
  /// Wattcycle hub MAC for ESP-NOW unicast, e.g. "24:6F:28:25:18:14".
  const char* espNowPeerMac = "";
  /// Optional 16+ char shared ESP-NOW PMK/LMK (empty = plaintext).
  const char* espNowPmk = "";
  /// Optional fixed Wi-Fi channel 1–13 (0 = scan SSID). Same channel as Wattcycle AP.
  uint8_t espNowChannel = 0;
  uint32_t sppReconnectIntervalMs = 5000;
  uint32_t sppConnectTimeoutMs = 20000;
};

class AppConfigFactory {
 public:
  static AppConfig fromBuildFlags();
  static bool hasWifiCredentials(const AppConfig& config);
  static bool hasEspNowPeer(const AppConfig& config);
  static bool hasBtTarget(const AppConfig& config);
};

}  // namespace xt369p::config
