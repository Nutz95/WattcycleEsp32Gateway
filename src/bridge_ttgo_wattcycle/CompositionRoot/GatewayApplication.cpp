#include "CompositionRoot/GatewayApplication.h"

#include "Config/TimingConstants.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

namespace wattcycle::composition {

GatewayApplication::GatewayApplication(const config::AppConfig& appConfig,
                                       bms::IBmsBleClient& bleClient,
                                       telemetry::ITelemetryStore& telemetryStore,
                                       espnow_tx::IEspNowTelemetryPublisher& espNowPublisher,
                                       display::IStatusDisplay& statusDisplay)
    : appConfig_(appConfig),
      telemetryStore_(telemetryStore),
      telemetryPoller_(bleClient, telemetryStore, appConfig),
      espNowPublisher_(espNowPublisher),
      statusDisplay_(statusDisplay) {}

bool GatewayApplication::begin() {
  statusDisplay_.begin();
  buttonNavigator_.begin();
  espHealthSampler_.begin();
#ifndef UNIT_TEST
  lastInputMs_ = millis();
#endif
  telemetryStore_.setBleState(false, appConfig_.bmsBleAddress, "waiting_radio");
  renderCurrentDisplay();

#ifndef UNIT_TEST
  Serial.printf("Free heap before ESP-NOW: %u\n", ESP.getFreeHeap());
#endif
  const bool espNowOk =
      espNowPublisher_.begin(appConfig_.wifiSsid, appConfig_.espNowPeerMac,
                             appConfig_.espNowChannel, appConfig_.espNowPmk);
  telemetryStore_.setEspNowState(espNowOk, espNowPublisher_.channel(), appConfig_.espNowPeerMac,
                                 espNowOk ? "" : espNowPublisher_.lastError());
#ifndef UNIT_TEST
  Serial.printf("ESP-NOW begin=%s ch=%u err='%s'\n", espNowOk ? "ok" : "fail",
                espNowPublisher_.channel(), espNowPublisher_.lastError());
  delay(300);
#endif

  telemetryPoller_.begin();
  startDisplayTask();
  startBleTask();
#ifndef UNIT_TEST
  Serial.println(F("Boot: ESP-NOW + NimBLE BMS bridge (no web / no STA join)"));
#endif
  return espNowOk;
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
#ifndef UNIT_TEST
  const uint32_t nowMs = millis();
#else
  const uint32_t nowMs = 0;
#endif
  maybePublishEspNow(nowMs);
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

void GatewayApplication::notifyDisplay() {
  lastDisplayFingerprint_ = 0;
#ifndef UNIT_TEST
  if (displayTaskHandle_ != nullptr) {
    xTaskNotifyGive(displayTaskHandle_);
  }
#endif
}

void GatewayApplication::handleButtons(uint32_t nowMs) {
  const display::ButtonEvent event = buttonNavigator_.poll(nowMs);
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

void GatewayApplication::renderCurrentDisplay() {
  statusDisplay_.render(telemetryStore_);
}

uint32_t GatewayApplication::displayFingerprint() const {
  const auto battery = telemetryStore_.battery();
  const auto esp = telemetryStore_.espHealth();
  const auto gateway = telemetryStore_.status();
  return (static_cast<uint32_t>(statusDisplay_.pageIndex()) << 24) ^ battery.updatedAtMs ^
         (static_cast<uint32_t>(battery.stateOfChargePercent) << 16) ^
         (static_cast<uint32_t>(esp.cpuCore0Percent) << 8) ^ esp.cpuCore1Percent ^
         (gateway.espNowReady ? 1u : 0u) ^ (gateway.bleConnected ? 2u : 0u) ^
         gateway.espNowChannel;
}

#ifndef UNIT_TEST

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

void GatewayApplication::startBleTask() {}

void GatewayApplication::startDisplayTask() {}

#endif

}  // namespace wattcycle::composition
