#include "CompositionRoot/GatewayApplication.h"

#include "Config/TimingConstants.h"

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
  refreshWifiStatus(millis());
  if (!wifiOk) {
    statusDisplay_.render(telemetryStore_);
    return false;
  }

  otaUpdater_.begin(appConfig_.otaHostname);
  webGateway_.begin(appConfig_.webServerPort);
  telemetryPoller_.begin();
  startBleTask();
  refreshWifiStatus(millis());
  statusDisplay_.render(telemetryStore_);
#ifndef UNIT_TEST
  Serial.printf("Web UI: http://%s:%u/\n", telemetryStore_.status().wifiIp,
                appConfig_.webServerPort);
#endif
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
  refreshWifiStatus(nowMs);
  maybeRefreshDisplay(nowMs);
}

void GatewayApplication::refreshWifiStatus(uint32_t nowMs) {
  if (lastWifiStatusMs_ != 0 &&
      (nowMs - lastWifiStatusMs_) < config::TimingConstants::kWifiStatusRefreshMs) {
    return;
  }
  char ip[16] = {};
  wifiConnector_.copyIpAddress(ip, sizeof(ip));
  telemetryStore_.setWifiState(wifiConnector_.isConnected(), ip);
  lastWifiStatusMs_ = nowMs;
}

void GatewayApplication::maybeRefreshDisplay(uint32_t nowMs) {
  if (lastDisplayMs_ != 0 &&
      (nowMs - lastDisplayMs_) < config::TimingConstants::kDisplayRefreshMs) {
    return;
  }
  statusDisplay_.render(telemetryStore_);
  lastDisplayMs_ = nowMs;
}

#ifndef UNIT_TEST

void GatewayApplication::startBleTask() {
  if (bleTaskHandle_ != nullptr) {
    return;
  }
  xTaskCreatePinnedToCore(
      &GatewayApplication::bleTaskTrampoline, "blePoll",
      config::TimingConstants::kBleTaskStackWords, this,
      config::TimingConstants::kBleTaskPriority, &bleTaskHandle_,
      config::TimingConstants::kRadioCoreId);
}

void GatewayApplication::bleTaskTrampoline(void* context) {
  static_cast<GatewayApplication*>(context)->bleTaskLoop();
}

void GatewayApplication::bleTaskLoop() {
  for (;;) {
    telemetryPoller_.loop(millis());
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

#else

void GatewayApplication::startBleTask() {}

#endif

}  // namespace wattcycle::composition
