#include "Config/AppConfig.h"

#include <cstring>

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif
#ifndef ESPNOW_BMS_BRIDGE_MAC
#define ESPNOW_BMS_BRIDGE_MAC ""
#endif
#ifndef ESPNOW_BRIDGE_MAC
#define ESPNOW_BRIDGE_MAC ""
#endif
#ifndef ESPNOW_PMK
#define ESPNOW_PMK ""
#endif
#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "wattcycle-gateway"
#endif
#ifndef WEB_SERVER_PORT
#define WEB_SERVER_PORT 6789
#endif
#ifndef POSIX_TIMEZONE
#define POSIX_TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#endif

namespace wattcycle::config {

AppConfig AppConfigFactory::fromBuildFlags() {
  AppConfig config;
  config.wifiSsid = WIFI_SSID;
  config.wifiPassword = WIFI_PASS;
  config.espNowBmsBridgeMac = ESPNOW_BMS_BRIDGE_MAC;
  // Keep ESPNOW_BRIDGE_MAC as the XT369P peer name for continuity with Phase 1.
  config.espNowXtBridgeMac = ESPNOW_BRIDGE_MAC;
  config.espNowPmk = ESPNOW_PMK;
  config.otaHostname = OTA_HOSTNAME;
  config.posixTimeZone = POSIX_TIMEZONE;
  config.webServerPort = static_cast<uint16_t>(WEB_SERVER_PORT);
  return config;
}

bool AppConfigFactory::hasWifiCredentials(const AppConfig& config) {
  return config.wifiSsid != nullptr && config.wifiSsid[0] != '\0' &&
         config.wifiPassword != nullptr && config.wifiPassword[0] != '\0';
}

}  // namespace wattcycle::config
