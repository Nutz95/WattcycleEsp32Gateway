#include "Config/AppConfig.h"

#include <cstdlib>

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef ECOFLOW_BLE_ADDRESS
#define ECOFLOW_BLE_ADDRESS ""
#endif
#ifndef ECOFLOW_SERIAL
#define ECOFLOW_SERIAL ""
#endif
#ifndef ECOFLOW_USER_ID
#define ECOFLOW_USER_ID ""
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

namespace ecoflow::config {

AppConfig AppConfigFactory::fromBuildFlags() {
  AppConfig config;
  config.wifiSsid = WIFI_SSID;
  config.ecoflowBleAddress = ECOFLOW_BLE_ADDRESS;
  config.ecoflowSerial = ECOFLOW_SERIAL;
  config.ecoflowUserId = ECOFLOW_USER_ID;
  config.espNowPeerMac = ESPNOW_PEER_MAC;
  config.espNowPmk = ESPNOW_PMK;
  const int channel = std::atoi(ESPNOW_CHANNEL);
  config.espNowChannel =
      (channel >= 1 && channel <= 13) ? static_cast<uint8_t>(channel) : static_cast<uint8_t>(0);
  return config;
}

}  // namespace ecoflow::config
