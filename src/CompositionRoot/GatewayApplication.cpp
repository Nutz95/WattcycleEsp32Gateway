#include "CompositionRoot/GatewayApplication.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

namespace wattcycle::composition {

GatewayApplication::GatewayApplication(const config::AppConfig& appConfig,
                                       wifi::IWifiConnector& wifiConnector,
                                       ota::IOtaUpdater& otaUpdater,
                                       bms::IBmsBleClient& bleClient,
                                       telemetry::ITelemetryStore& telemetryStore,
                                       web::IWebGateway& webGateway,
                                       display::IStatusDisplay& statusDisplay)
    : appConfig_(appConfig),
      wifiConnector_(wifiConnector),
      otaUpdater_(otaUpdater),
      bleClient_(bleClient),
      telemetryStore_(telemetryStore),
      telemetryPoller_(bleClient, telemetryStore, appConfig),
      webGateway_(webGateway),
      statusDisplay_(statusDisplay) {}

bool GatewayApplication::begin() {
  statusDisplay_.begin();

  if (!config::AppConfigFactory::hasWifiCredentials(appConfig_)) {
    telemetryStore_.setWifiState(false, "");
    telemetryStore_.setBleState(false, appConfig_.bmsBleAddress,
                                "WIFI_SSID/WIFI_PASS missing");
    statusDisplay_.render(telemetryStore_);
    return false;
  }

  const bool wifiOk = wifiConnector_.connect(
      appConfig_.wifiSsid, appConfig_.wifiPassword, appConfig_.wifiConnectTimeoutMs);
  refreshWifiStatus();
  if (!wifiOk) {
    statusDisplay_.render(telemetryStore_);
    return false;
  }

  otaUpdater_.begin(appConfig_.otaHostname);
  webGateway_.begin(appConfig_.webServerPort);
  telemetryPoller_.begin();
  statusDisplay_.render(telemetryStore_);
  return true;
}

void GatewayApplication::serviceNetwork() {
  wifiConnector_.loop();
  otaUpdater_.loop();
  webGateway_.loop();
}

void GatewayApplication::loop() {
  const uint32_t nowMs = millis();
  serviceNetwork();
  telemetryPoller_.loop(nowMs);
  refreshWifiStatus();
  maybeRefreshDisplay(nowMs);
}

void GatewayApplication::refreshWifiStatus() {
  char ip[16] = {};
  wifiConnector_.copyIpAddress(ip, sizeof(ip));
  telemetryStore_.setWifiState(wifiConnector_.isConnected(), ip);
}

void GatewayApplication::maybeRefreshDisplay(uint32_t nowMs) {
  constexpr uint32_t kDisplayRefreshMs = 2000;
  if (lastDisplayMs_ != 0 && (nowMs - lastDisplayMs_) < kDisplayRefreshMs) {
    return;
  }
  statusDisplay_.render(telemetryStore_);
  lastDisplayMs_ = nowMs;
}

}  // namespace wattcycle::composition
