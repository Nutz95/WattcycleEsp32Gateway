#include "CompositionRoot/GatewayApplication.h"

#include "Config/TimingConstants.h"

#include <Arduino.h>

namespace wattcycle::composition {

GatewayApplication::GatewayApplication(const config::AppConfig& appConfig,
                                       bms::IBmsBleClient& bleClient,
                                       telemetry::ITelemetryStore& telemetryStore,
                                       espnow_tx::IEspNowTelemetryPublisher& espNowPublisher,
                                       display::IStatusDisplay& statusDisplay,
                                       display::IButtonSource& buttonSource)
    : appConfig_(appConfig),
      telemetryStore_(telemetryStore),
      telemetryPoller_(bleClient, telemetryStore, appConfig),
      espNowPublisher_(espNowPublisher),
      statusDisplay_(statusDisplay),
      buttonSource_(buttonSource),
      displayUi_(*this) {}

bool GatewayApplication::begin() {
  buttonSource_.begin();
  espHealthSampler_.begin();
  lastInputMs_ = millis();
  telemetryStore_.setBleState(false, appConfig_.bmsBleAddress, "waiting_radio");

  // Display first — verify TFT without BLE pack / hub nearby.
  statusDisplay_.begin();
  renderCurrentDisplay();

  bool espNowOk = false;
  const bool havePeer =
      appConfig_.espNowPeerMac != nullptr && appConfig_.espNowPeerMac[0] != '\0';
  if (havePeer) {
    Serial.printf("Free heap before ESP-NOW: %u\n", ESP.getFreeHeap());
    espNowOk = espNowPublisher_.begin(appConfig_.wifiSsid, appConfig_.espNowPeerMac,
                                      appConfig_.espNowChannel, appConfig_.espNowPmk);
    telemetryStore_.setEspNowState(espNowOk, espNowPublisher_.channel(), appConfig_.espNowPeerMac,
                                   espNowOk ? "" : espNowPublisher_.lastError());
    Serial.printf("ESP-NOW begin=%s ch=%u err='%s'\n", espNowOk ? "ok" : "fail",
                  espNowPublisher_.channel(), espNowPublisher_.lastError());
    delay(300);
    notifyDisplay();
  } else {
    telemetryStore_.setEspNowState(false, 0, "", "no_peer_mac");
    Serial.println(F("ESP-NOW skipped (set ESPNOW_PEER_MAC)"));
  }

  telemetryPoller_.begin();

  boards::ui::AppCoreDisplayUiTaskConfig displayUiConfig;
  displayUiConfig.taskName = "displayUi";
  displayUiConfig.stackWords = config::TimingConstants::kDisplayTaskStackWords;
  displayUiConfig.priority = config::TimingConstants::kDisplayTaskPriority;
  displayUiConfig.refreshMs = config::TimingConstants::kDisplayRefreshMs;
  displayUiConfig.coreId = config::TimingConstants::kAppCoreId;
  displayUi_.start(displayUiConfig);

  startBleTask();
  Serial.println(F("Boot: display(APP_CPU) + NimBLE BMS (PRO_CPU); ESP-NOW optional"));
  return true;
}

void GatewayApplication::maybePublishEspNow(uint32_t nowMs) {
  if (!espNowPublisher_.isReady()) {
    return;
  }
  if (lastEspNowPublishMs_ != 0 &&
      (nowMs - lastEspNowPublishMs_) < config::TimingConstants::kEspNowPublishMs) {
    return;
  }
  lastEspNowPublishMs_ = nowMs;
  espNowPublisher_.publish(telemetryStore_);
}

void GatewayApplication::loop() {
  const uint32_t nowMs = millis();
  maybePublishEspNow(nowMs);
  sampleEspHealth();
  handleButtons(nowMs);
  maybeSleepDisplay(nowMs);
  delay(config::TimingConstants::kAppLoopDelayMs);
}

void GatewayApplication::sampleEspHealth() {
  if (!espHealthSampler_.sample()) {
    return;
  }
  telemetryStore_.updateEspHealth(espHealthSampler_.snapshot());
}

void GatewayApplication::notifyDisplay() {
  lastDisplayFingerprint_ = 0;
  displayUi_.notify();
}

void GatewayApplication::handleButtons(uint32_t nowMs) {
  const display::ButtonEvent event = buttonSource_.poll(nowMs);
  if (!event.anyPress) {
    return;
  }
  if (displayAsleep_) {
    wakeDisplay(nowMs);
    return;
  }

  lastInputMs_ = nowMs;
  if (event.action == display::ButtonAction::Sleep) {
    sleepDisplay();
    return;
  }
  if (event.action == display::ButtonAction::Previous) {
    statusDisplay_.previousPage();
  } else if (event.action == display::ButtonAction::Next) {
    statusDisplay_.nextPage();
  }
  notifyDisplay();
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
  if (displayAsleep_) {
    return;
  }
  if ((nowMs - lastInputMs_) < config::TimingConstants::kDisplaySleepMs) {
    return;
  }
  sleepDisplay();
}

void GatewayApplication::renderCurrentDisplay() {
  statusDisplay_.render(telemetryStore_);
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
  const auto battery = telemetryStore_.battery();
  const auto esp = telemetryStore_.espHealth();
  const auto gateway = telemetryStore_.status();
  const uint32_t bleRetryTick =
      gateway.bleConnected ? 0u : static_cast<uint32_t>(millis() / 1000u);
  return (static_cast<uint32_t>(statusDisplay_.pageIndex()) << 24) ^ battery.updatedAtMs ^
         (static_cast<uint32_t>(battery.stateOfChargePercent) << 16) ^
         (static_cast<uint32_t>(esp.cpuCore0Percent) << 8) ^ esp.cpuCore1Percent ^
         (gateway.espNowReady ? 1u : 0u) ^ (gateway.bleConnected ? 2u : 0u) ^
         gateway.espNowChannel ^ gateway.bleConnectAttempts ^ bleRetryTick;
}

void GatewayApplication::startBleTask() {
  if (bleTaskHandle_ != nullptr) {
    return;
  }
  xTaskCreatePinnedToCore(&GatewayApplication::bleTaskTrampoline, "bleRadio",
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
    const uint32_t delayMs = telemetryStore_.status().bleConnected ? 100 : 50;
    vTaskDelay(pdMS_TO_TICKS(delayMs));
  }
}

}  // namespace wattcycle::composition
