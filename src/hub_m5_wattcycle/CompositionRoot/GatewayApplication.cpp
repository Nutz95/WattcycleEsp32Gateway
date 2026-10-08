#include "CompositionRoot/GatewayApplication.h"

#include "Config/TimingConstants.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

namespace wattcycle::composition {

GatewayApplication::GatewayApplication(const config::AppConfig& appConfig,
                                       wifi::IWifiConnector& wifiConnector,
                                       ota::IOtaUpdater& otaUpdater,
                                       telemetry::ITelemetryStore& telemetryStore,
                                       web::IWebGateway& webGateway,
                                       display::IStatusDisplay& statusDisplay,
                                       auth::IAuthPhysicalConfirm& authConfirm,
                                       espnow_rx::IEspNowTelemetryReceiver& espNowReceiver)
    : appConfig_(appConfig),
      wifiConnector_(wifiConnector),
      otaUpdater_(otaUpdater),
      telemetryStore_(telemetryStore),
      webGateway_(webGateway),
      statusDisplay_(statusDisplay),
      authConfirm_(authConfirm),
      espNowReceiver_(espNowReceiver) {}

bool GatewayApplication::begin() {
  statusDisplay_.begin();
  buttonNavigator_.begin();
  espHealthSampler_.begin();
#ifndef UNIT_TEST
  bootMs_ = millis();
  lastInputMs_ = bootMs_;
#else
  bootMs_ = 0;
  lastInputMs_ = 0;
#endif

  if (!config::AppConfigFactory::hasWifiCredentials(appConfig_)) {
    telemetryStore_.setWifiState(false, "");
    telemetryStore_.setBleState(false, "", "WIFI_SSID/WIFI_PASS missing");
    renderCurrentDisplay();
    startDisplayTask();
    return false;
  }

  const bool wifiOk = wifiConnector_.connect(
      appConfig_.wifiSsid, appConfig_.wifiPassword, appConfig_.wifiConnectTimeoutMs);
  refreshWifiStatus(millis());

  if (wifiOk) {
    otaUpdater_.begin(appConfig_.otaHostname);
    webGateway_.begin(appConfig_.webServerPort);
    telemetryStore_.setWebPort(appConfig_.webServerPort);
#ifndef UNIT_TEST
    Serial.printf("Web UI: http://%s:%u/\n", telemetryStore_.status().wifiIp,
                  appConfig_.webServerPort);
#endif
  }

  sampleEspHealth();
  renderCurrentDisplay();
  startDisplayTask();
  return wifiOk;
}

void GatewayApplication::maybeStartEspNow(uint32_t nowMs) {
  if (espNowAttempted_ || espNowReceiver_.isReady()) {
    return;
  }
  if ((nowMs - bootMs_) < 3000u) {
    return;
  }
  if (!wifiConnector_.isConnected()) {
    return;
  }
  espNowAttempted_ = true;
#ifndef UNIT_TEST
  Serial.printf("Starting multi-peer ESP-NOW heap=%u\n", ESP.getFreeHeap());
#endif
  if (!espNowReceiver_.begin(appConfig_.espNowBmsBridgeMac, appConfig_.espNowXtBridgeMac,
                             appConfig_.espNowPmk)) {
#ifndef UNIT_TEST
    Serial.println(F("ESP-NOW off - Wi-Fi/web kept"));
#endif
  }
}

void GatewayApplication::serviceNetwork() {
  wifiConnector_.loop();
  otaUpdater_.loop();
  webGateway_.loop();
  espNowReceiver_.loop();
}

void GatewayApplication::loop() {
#ifndef UNIT_TEST
  const uint32_t nowMs = millis();
#else
  const uint32_t nowMs = 0;
#endif
  serviceNetwork();
  maybeStartEspNow(nowMs);
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

void GatewayApplication::notifyDisplay() {
  lastDisplayFingerprint_ = 0;
#ifndef UNIT_TEST
  if (displayTaskHandle_ != nullptr) {
    xTaskNotifyGive(displayTaskHandle_);
  }
#endif
}

bool GatewayApplication::authUiActive() const {
  return authConfirm_.prompt().kind != auth::AuthPromptKind::None;
}

bool GatewayApplication::handleAuthUi(const display::ButtonEvent& event, uint32_t nowMs) {
  const bool wasActive = authUiActive();
  authConfirm_.onCancelHeld(event.action == display::ButtonAction::Cancel, nowMs);
  if (!authUiActive()) {
    return false;
  }

  if (displayAsleep_ || !wasActive) {
    wakeDisplay(nowMs);
    notifyDisplay();
  } else {
    lastInputMs_ = nowMs;
  }

  if (event.action == display::ButtonAction::Confirm ||
      event.action == display::ButtonAction::Next) {
    authConfirm_.onConfirmPressed();
    notifyDisplay();
  } else if (event.action == display::ButtonAction::Previous ||
             event.action == display::ButtonAction::Cancel) {
    authConfirm_.onCancelPressed();
    notifyDisplay();
  }
  return true;
}

void GatewayApplication::handleButtons(uint32_t nowMs) {
  const display::ButtonEvent event = buttonNavigator_.poll(nowMs);
  if (handleAuthUi(event, nowMs)) {
    return;
  }
  if (!event.anyPress) {
    return;
  }
  if (displayAsleep_) {
    wakeDisplay(nowMs);
    return;
  }

  lastInputMs_ = nowMs;
  if (event.action == display::ButtonAction::Previous) {
    statusDisplay_.previousPage();
  } else if (event.action == display::ButtonAction::Next) {
    statusDisplay_.nextPage();
  }
  notifyDisplay();
}

void GatewayApplication::sleepDisplay() {
  if (displayAsleep_ || authUiActive()) {
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
  if (displayAsleep_ || authUiActive()) {
    return;
  }
  if ((nowMs - lastInputMs_) < config::TimingConstants::kDisplaySleepMs) {
    return;
  }
  sleepDisplay();
}

void GatewayApplication::renderCurrentDisplay() {
  if (authUiActive()) {
    statusDisplay_.renderAuthPrompt(authConfirm_.prompt());
    return;
  }
  statusDisplay_.render(telemetryStore_);
}

uint32_t GatewayApplication::displayFingerprint() const {
  if (authUiActive()) {
    return 0xA0000000u | static_cast<uint32_t>(authConfirm_.prompt().kind);
  }
  const auto battery = telemetryStore_.battery();
  const auto solar = telemetryStore_.solar();
  const auto esp = telemetryStore_.espHealth();
  const auto gateway = telemetryStore_.status();
  // Coarse fingerprint: avoid redrawing every ESP-NOW seq tick (canvas still
  // double-buffers, but less CPU / SPI thrash).
  const uint32_t solarMilliV = static_cast<uint32_t>(solar.voltageV * 10.0f);
  const uint32_t batteryDeciV = static_cast<uint32_t>(battery.moduleVoltage * 10.0f);
  return (static_cast<uint32_t>(statusDisplay_.pageIndex()) << 24) ^
         (static_cast<uint32_t>(battery.stateOfChargePercent) << 16) ^
         (batteryDeciV << 8) ^ solarMilliV ^
         (static_cast<uint32_t>(esp.cpuCore0Percent) << 4) ^
         (gateway.wifiConnected ? 1u : 0u) ^ (gateway.bleConnected ? 2u : 0u) ^
         (solar.linkFresh ? 4u : 0u) ^ (solar.meterValid ? 8u : 0u);
}

#ifndef UNIT_TEST

void GatewayApplication::startDisplayTask() {
  if (displayTaskHandle_ != nullptr) {
    return;
  }
  xTaskCreatePinnedToCore(&GatewayApplication::displayTaskTrampoline, "displayUi",
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
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      continue;
    }

    const uint32_t fingerprint = displayFingerprint();
    if (fingerprint != lastDisplayFingerprint_) {
      renderCurrentDisplay();
      lastDisplayFingerprint_ = fingerprint;
    }

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(config::TimingConstants::kDisplayRefreshMs));
  }
}

#else

void GatewayApplication::startDisplayTask() {}

#endif

}  // namespace wattcycle::composition
