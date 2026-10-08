#include "Config/AppConfig.h"

#include <cstdlib>
#include <cstring>

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef BMS_BLE_ADDRESS
#define BMS_BLE_ADDRESS ""
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
#ifndef BMS_POLL_INTERVAL_MS
#define BMS_POLL_INTERVAL_MS 2000
#endif

namespace wattcycle::config {

AppConfig AppConfigFactory::fromBuildFlags() {
  AppConfig config;
  config.wifiSsid = WIFI_SSID;
  config.bmsBleAddress = BMS_BLE_ADDRESS;
  config.espNowPeerMac = ESPNOW_PEER_MAC;
  config.espNowPmk = ESPNOW_PMK;
  const int channel = std::atoi(ESPNOW_CHANNEL);
  config.espNowChannel =
      (channel >= 1 && channel <= 13) ? static_cast<uint8_t>(channel) : static_cast<uint8_t>(0);
  config.bmsPollIntervalMs = static_cast<uint32_t>(BMS_POLL_INTERVAL_MS);
  return config;
}

bool AppConfigFactory::hasBmsAddress(const AppConfig& config) {
  return config.bmsBleAddress != nullptr && std::strlen(config.bmsBleAddress) >= 17;
}

bool AppConfigFactory::hasEspNowPeer(const AppConfig& config) {
  return config.espNowPeerMac != nullptr && std::strlen(config.espNowPeerMac) >= 17;
}

}  // namespace wattcycle::config
