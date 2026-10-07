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
  buttonNavigator_.begin();
  espHealthSampler_.begin();
  lastInputMs_ = millis();

  if (!config::AppConfigFactory::hasWifiCredentials(appConfig_)) {
    telemetryStore_.setWifiState(false, "");
    telemetryStore_.setBleState(false, appConfig_.bmsBleAddress,
                                "WIFI_SSID/WIFI_PASS missing");
    statusDisplay_.render(telemetryStore_);
    startDisplayTask();
    return false;
  }

  const bool wifiOk = wifiConnector_.connect(
      appConfig_.wifiSsid, appConfig_.wifiPassword, appConfig_.wifiConnectTimeoutMs);
  refreshWifiStatus(millis());
  if (!wifiOk) {
    statusDisplay_.render(telemetryStore_);
    startDisplayTask();
    return false;
  }

  otaUpdater_.begin(appConfig_.otaHostname);
  webGateway_.begin(appConfig_.webServerPort);
  telemetryPoller_.begin();
  startBleTask();
  refreshWifiStatus(millis());
  sampleEspHealth();
  statusDisplay_.render(telemetryStore_);
  startDisplayTask();
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
  sampleEspHealth();
  handleButtons(nowMs);
  maybeSleepDisplay(nowMs);
#ifndef UNIT_TEST
  delay(config::TimingConstants::kAppLoopDelayMs);
#endif
}

void GatewayApplication::sampleEspHealth() {
  if (!espHealthSampler_.sample()) {
    return;
  }
  telemetryStore_.updateEspHealth(espHealthSampler_.snapshot());
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

void GatewayApplication::handleButtons(uint32_t nowMs) {
  const display::ButtonEvent event = buttonNavigator_.poll(nowMs);
  if (!event.anyPress) {
    return;
  }

  if (displayAsleep_) {
    // First press while sleeping only wakes the panel (no page change).
    wakeDisplay(nowMs);
    return;
  }

  lastInputMs_ = nowMs;
  if (event.action == display::ButtonAction::Previous) {
    statusDisplay_.previousPage();
  } else if (event.action == display::ButtonAction::Next) {
    statusDisplay_.nextPage();
  }
  lastDisplayFingerprint_ = 0;
#ifndef UNIT_TEST
  if (displayTaskHandle_ != nullptr) {
    xTaskNotifyGive(displayTaskHandle_);
  }
#endif
}

void GatewayApplication::sleepDisplay() {
  if (displayAsleep_) {
    return;
  }
  displayAsleep_ = true;
  statusDisplay_.setBacklight(false);
#ifndef UNIT_TEST
  if (displayTaskHandle_ != nullptr) {
    vTaskSuspend(displayTaskHandle_);
  }
#endif
}

void GatewayApplication::wakeDisplay(uint32_t nowMs) {
  lastInputMs_ = nowMs;
  if (!displayAsleep_) {
    return;
  }
  displayAsleep_ = false;
  statusDisplay_.setBacklight(true);
  lastDisplayFingerprint_ = 0;
#ifndef UNIT_TEST
  if (displayTaskHandle_ != nullptr) {
    vTaskResume(displayTaskHandle_);
    xTaskNotifyGive(displayTaskHandle_);
  }
#endif
}

void GatewayApplication::maybeSleepDisplay(uint32_t nowMs) {
  if (displayAsleep_) {
    return;
  }
  if ((nowMs - lastInputMs_) < config::TimingConstants::kDisplaySleepMs) {
    return;
  }
  sleepDisplay();
}

uint32_t GatewayApplication::displayFingerprint() const {
  const auto battery = telemetryStore_.battery();
  const auto esp = telemetryStore_.espHealth();
  const auto gateway = telemetryStore_.status();
  return (static_cast<uint32_t>(statusDisplay_.pageIndex()) << 24) ^ battery.updatedAtMs ^
         (static_cast<uint32_t>(battery.stateOfChargePercent) << 16) ^
         (static_cast<uint32_t>(esp.cpuCore0Percent) << 8) ^ esp.cpuCore1Percent ^
         (gateway.wifiConnected ? 1u : 0u) ^ (gateway.bleConnected ? 2u : 0u);
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

void GatewayApplication::startDisplayTask() {
  if (displayTaskHandle_ != nullptr) {
    return;
  }
  xTaskCreatePinnedToCore(
      &GatewayApplication::displayTaskTrampoline, "displayUi",
      config::TimingConstants::kDisplayTaskStackWords, this,
      config::TimingConstants::kDisplayTaskPriority, &displayTaskHandle_,
      config::TimingConstants::kAppCoreId);
}

void GatewayApplication::displayTaskTrampoline(void* context) {
  static_cast<GatewayApplication*>(context)->displayTaskLoop();
}

void GatewayApplication::displayTaskLoop() {
  for (;;) {
    if (displayAsleep_) {
      // Suspended by sleepDisplay(); if we ever run while flagged asleep, wait.
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      continue;
    }

    const uint32_t fingerprint = displayFingerprint();
    if (fingerprint != lastDisplayFingerprint_) {
      statusDisplay_.render(telemetryStore_);
      lastDisplayFingerprint_ = fingerprint;
    }

    // Wake early on page change / explicit wake notify.
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(config::TimingConstants::kDisplayRefreshMs));
  }
}

#else

void GatewayApplication::startBleTask() {}

void GatewayApplication::startDisplayTask() {}

#endif

}  // namespace wattcycle::composition
