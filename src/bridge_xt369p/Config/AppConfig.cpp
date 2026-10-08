#include "Config/AppConfig.h"

#include <cstdlib>
#include <cstring>

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef XT369P_BT_ADDRESS
#define XT369P_BT_ADDRESS ""
#endif
#ifndef ESPNOW_PEER_MAC
#define ESPNOW_PEER_MAC ""
#endif
#ifndef ESPNOW_PMK
#define ESPNOW_PMK ""
#endif
#ifndef ESPNOW_CHANNEL
#define ESPNOW_CHANNEL "0"
#endif
#ifndef SPP_RECONNECT_INTERVAL_MS
#define SPP_RECONNECT_INTERVAL_MS 5000
#endif

namespace xt369p::config {

AppConfig AppConfigFactory::fromBuildFlags() {
  AppConfig config;
  config.wifiSsid = WIFI_SSID;
  config.xt369pBtAddress = XT369P_BT_ADDRESS;
  config.espNowPeerMac = ESPNOW_PEER_MAC;
  config.espNowPmk = ESPNOW_PMK;
  const int channel = std::atoi(ESPNOW_CHANNEL);
  config.espNowChannel =
      (channel >= 1 && channel <= 13) ? static_cast<uint8_t>(channel) : static_cast<uint8_t>(0);
  config.sppReconnectIntervalMs = static_cast<uint32_t>(SPP_RECONNECT_INTERVAL_MS);
  return config;
}

bool AppConfigFactory::hasWifiCredentials(const AppConfig& config) {
  return config.wifiSsid != nullptr && config.wifiSsid[0] != '\0';
}

bool AppConfigFactory::hasEspNowPeer(const AppConfig& config) {
  return config.espNowPeerMac != nullptr && std::strlen(config.espNowPeerMac) >= 17;
}

bool AppConfigFactory::hasBtTarget(const AppConfig& config) {
  return (config.xt369pBtAddress != nullptr && config.xt369pBtAddress[0] != '\0') ||
         (config.sppDeviceName != nullptr && config.sppDeviceName[0] != '\0');
}

}  // namespace xt369p::config
