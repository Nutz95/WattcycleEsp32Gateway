#include "CompositionRoot/GatewayApplication.h"

#include "Config/TimingConstants.h"

#include <Arduino.h>
#include <esp_chip_info.h>

namespace ecoflow::composition {

GatewayApplication::GatewayApplication(const config::AppConfig& appConfig,
                                       ble::IEcoFlowBleClient& bleClient,
                                       espnow_tx::IEspNowTelemetryPublisher& espNowPublisher,
                                       esp_sys::EspHealthSampler& espHealth,
                                       display::IStatusDisplay& statusDisplay,
                                       boards::input::IButtonSource& buttonSource)
    : appConfig_(appConfig),
      bleClient_(bleClient),
      espNowPublisher_(espNowPublisher),
      espHealth_(espHealth),
      statusDisplay_(statusDisplay),
      buttonSource_(buttonSource),
      displayUi_(*this) {}

bool GatewayApplication::begin() {
  esp_chip_info_t chipInfo{};
  esp_chip_info(&chipInfo);

  Serial.println(F("EcoFlow DELTA 3 bridge — BLE + ESP-NOW TX"));
  Serial.printf("Chip model: %s  cores=%u  revision=%u\n", ESP.getChipModel(), chipInfo.cores,
                chipInfo.revision);
  Serial.printf("Flash: %u KB  Free heap: %u\n", ESP.getFlashChipSize() / 1024U,
                ESP.getFreeHeap());
  Serial.printf("Config present: ble=%s serial=%s userId=%s peer=%s\n",
                (appConfig_.ecoflowBleAddress && appConfig_.ecoflowBleAddress[0]) ? "yes" : "no",
                (appConfig_.ecoflowSerial && appConfig_.ecoflowSerial[0]) ? "yes" : "no",
                (appConfig_.ecoflowUserId && appConfig_.ecoflowUserId[0]) ? "yes" : "no",
                (appConfig_.espNowPeerMac && appConfig_.espNowPeerMac[0]) ? "yes" : "no");

  buttonSource_.begin();
  espHealth_.begin();
  lastInputMs_ = millis();

  // Display first — UI bring-up without station / hub nearby.
  statusDisplay_.begin();
  renderCurrentDisplay();

  delay(config::TimingConstants::kRadioBringupDelayMs);
  if (appConfig_.espNowPeerMac != nullptr && appConfig_.espNowPeerMac[0] != '\0') {
    if (!espNowPublisher_.begin(appConfig_.wifiSsid, appConfig_.espNowPeerMac,
                                appConfig_.espNowChannel, appConfig_.espNowPmk)) {
      Serial.printf("ESP-NOW begin failed: %s\n", espNowPublisher_.lastError());
    }
  } else {
    Serial.println(F("ESP-NOW skipped (set ESPNOW_PEER_MAC)"));
  }
  notifyDisplay();

  boards::ui::AppCoreDisplayUiTaskConfig displayUiConfig;
  displayUiConfig.taskName = "ecoDisplay";
  displayUiConfig.stackWords = config::TimingConstants::kDisplayTaskStackWords;
  displayUiConfig.priority = config::TimingConstants::kDisplayTaskPriority;
  displayUiConfig.refreshMs = config::TimingConstants::kDisplayRefreshMs;
  displayUiConfig.coreId = config::TimingConstants::kAppCoreId;
  displayUi_.start(displayUiConfig);

  startBleTask();
  lastHeartbeatMs_ = millis();
  lastPublishMs_ = millis();
  Serial.println(F("Boot: display(APP_CPU) + EcoFlow BLE (PRO_CPU)"));
  return true;
}

void GatewayApplication::printHeartbeat(uint32_t nowMs) {
  ++heartbeatCount_;
  const auto bleSnapshot = bleClient_.snapshot();
  const auto health = espHealth_.snapshot();
  Serial.printf(
      "[hb %lu] up=%lus heap=%u cpu0=%u%% chip=%.0fC ble=%s auth=%s soc=%u acOut=%dW "
      "notify=%lu tel=%lu err='%s'\n",
      static_cast<unsigned long>(heartbeatCount_), static_cast<unsigned long>(nowMs / 1000U),
      ESP.getFreeHeap(), static_cast<unsigned>(health.cpuCore0Percent),
      static_cast<double>(health.chipTemperatureC), bleSnapshot.connected ? "up" : "down",
      bleSnapshot.authenticated ? "ok" : "--",
      static_cast<unsigned>(bleSnapshot.telemetry.socPercent),
      static_cast<int>(bleSnapshot.telemetry.acOutputW),
      static_cast<unsigned long>(bleSnapshot.notifyCount),
      static_cast<unsigned long>(bleSnapshot.telemetryCount), bleSnapshot.lastError);
}

void GatewayApplication::maybePublishEspNow(uint32_t nowMs) {
  if (!espNowPublisher_.isReady()) {
    return;
  }
  if (lastPublishMs_ != 0 &&
      (nowMs - lastPublishMs_) < config::TimingConstants::kEspNowPublishMs) {
    return;
  }
  lastPublishMs_ = nowMs;
  espHealth_.sample();
  const auto bleSnapshot = bleClient_.snapshot();
  espNowPublisher_.publish(bleSnapshot.telemetry, bleSnapshot.connected, bleSnapshot.telemetryCount,
                           bleSnapshot.lastError, espHealth_.snapshot());
}

void GatewayApplication::renderCurrentDisplay() {
  if (displayAsleep_) {
    return;
  }
  espHealth_.sample();
  display::DongleBridgeLink link;
  link.espNowReady = espNowPublisher_.isReady();
  link.channel = appConfig_.espNowChannel;
  link.peerMac = appConfig_.espNowPeerMac != nullptr ? appConfig_.espNowPeerMac : "";
  statusDisplay_.render(bleClient_.snapshot(), espHealth_.snapshot(), link);
}

bool GatewayApplication::isDisplayAsleep() const {
  return displayAsleep_;
}

void GatewayApplication::refreshIfDirty() {
  const uint32_t fingerprint = displayFingerprint();
  if (fingerprint == lastDisplayFingerprint_) {
    return;
  }
  renderCurrentDisplay();
  lastDisplayFingerprint_ = fingerprint;
}

uint32_t GatewayApplication::displayFingerprint() const {
  const auto bleSnapshot = bleClient_.snapshot();
  const auto health = espHealth_.snapshot();
  return (static_cast<uint32_t>(statusDisplay_.pageIndex()) << 24) ^
         (bleSnapshot.connected ? 1u : 0u) ^ (bleSnapshot.authenticated ? 2u : 0u) ^
         (espNowPublisher_.isReady() ? 4u : 0u) ^ bleSnapshot.telemetryCount ^
         (health.freeHeapBytes >> 10);
}

void GatewayApplication::notifyDisplay() {
  lastDisplayFingerprint_ = 0;
  displayUi_.notify();
}

void GatewayApplication::sleepDisplay() {
  if (displayAsleep_) {
    return;
  }
  displayAsleep_ = true;
  statusDisplay_.setBacklight(false);
  displayUi_.suspend();
}

void GatewayApplication::wakeDisplay(uint32_t nowMs) {
  lastInputMs_ = nowMs;
  if (!displayAsleep_) {
    return;
  }
  displayAsleep_ = false;
  statusDisplay_.setBacklight(true);
  lastDisplayFingerprint_ = 0;
  displayUi_.resume();
}

void GatewayApplication::maybeSleepDisplay(uint32_t nowMs) {
  if (displayAsleep_ || !statusDisplay_.isBacklightOn()) {
    return;
  }
  if ((nowMs - lastInputMs_) >= config::TimingConstants::kDisplaySleepMs) {
    sleepDisplay();
  }
}

void GatewayApplication::handleButtons(uint32_t nowMs) {
  const auto event = buttonSource_.poll(nowMs);
  if (!event.anyPress) {
    return;
  }
  if (displayAsleep_) {
    wakeDisplay(nowMs);
    return;
  }
  lastInputMs_ = nowMs;
  if (event.action == boards::input::ButtonAction::Sleep) {
    sleepDisplay();
    return;
  }
  if (event.action == boards::input::ButtonAction::Previous) {
    statusDisplay_.previousPage();
  } else if (event.action == boards::input::ButtonAction::Next) {
    statusDisplay_.nextPage();
  }
  notifyDisplay();
}

void GatewayApplication::loop() {
  espHealth_.sample();
  const uint32_t nowMs = millis();
  maybePublishEspNow(nowMs);
  handleButtons(nowMs);
  maybeSleepDisplay(nowMs);
  if (lastHeartbeatMs_ == 0 ||
      (nowMs - lastHeartbeatMs_) >= config::TimingConstants::kHeartbeatMs) {
    lastHeartbeatMs_ = nowMs;
    printHeartbeat(nowMs);
  }
  delay(config::TimingConstants::kAppLoopDelayMs);
}

void GatewayApplication::startBleTask() {
  if (bleTaskHandle_ != nullptr) {
    return;
  }
  xTaskCreatePinnedToCore(&GatewayApplication::bleTaskTrampoline, "ecoBle",
                          config::TimingConstants::kBleTaskStackWords, this,
                          config::TimingConstants::kBleTaskPriority, &bleTaskHandle_,
                          config::TimingConstants::kRadioCoreId);
}

void GatewayApplication::bleTaskTrampoline(void* context) {
  static_cast<GatewayApplication*>(context)->bleTaskLoop();
}

void GatewayApplication::bleTaskLoop() {
  vTaskDelay(pdMS_TO_TICKS(config::TimingConstants::kBleBringupDelayMs));
  const bool bleOk =
      bleClient_.begin(appConfig_.ecoflowBleAddress, appConfig_.ecoflowSerial,
                       appConfig_.ecoflowUserId);
  if (!bleOk) {
    const auto bleSnapshot = bleClient_.snapshot();
    Serial.printf("EcoFlow BLE begin failed: %s\n", bleSnapshot.lastError);
  }
  for (;;) {
    bleClient_.loop();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

}  // namespace ecoflow::composition
