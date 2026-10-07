#include "Config/AppConfig.h"

#include <cstring>

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif
#ifndef BMS_BLE_ADDRESS
#define BMS_BLE_ADDRESS ""
#endif
#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "wattcycle-gateway"
#endif
#ifndef WEB_SERVER_PORT
#define WEB_SERVER_PORT 6789
#endif
#ifndef BMS_POLL_INTERVAL_MS
#define BMS_POLL_INTERVAL_MS 2000
#endif

namespace wattcycle::config {

AppConfig AppConfigFactory::fromBuildFlags() {
  AppConfig config;
  config.wifiSsid = WIFI_SSID;
  config.wifiPassword = WIFI_PASS;
  config.bmsBleAddress = BMS_BLE_ADDRESS;
  config.otaHostname = OTA_HOSTNAME;
  config.webServerPort = static_cast<uint16_t>(WEB_SERVER_PORT);
  config.bmsPollIntervalMs = static_cast<uint32_t>(BMS_POLL_INTERVAL_MS);
  return config;
}

bool AppConfigFactory::hasWifiCredentials(const AppConfig& config) {
  return config.wifiSsid != nullptr && std::strlen(config.wifiSsid) > 0 &&
         config.wifiPassword != nullptr && std::strlen(config.wifiPassword) > 0;
}

bool AppConfigFactory::hasBmsAddress(const AppConfig& config) {
  return config.bmsBleAddress != nullptr && std::strlen(config.bmsBleAddress) >= 17;
}

}  // namespace wattcycle::config
