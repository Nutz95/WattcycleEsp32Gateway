#include "CompositionRoot/GatewayApplication.h"

#include "Config/TimingConstants.h"

#include <Arduino.h>
#include <esp_chip_info.h>

namespace ecoflow::composition {

GatewayApplication::GatewayApplication(const config::AppConfig& appConfig,
                                       ble::IEcoFlowBleClient& bleClient,
                                       espnow_tx::IEspNowTelemetryPublisher& espNowPublisher,
                                       esp_sys::EspHealthSampler& espHealth)
    : appConfig_(appConfig),
      bleClient_(bleClient),
      espNowPublisher_(espNowPublisher),
      espHealth_(espHealth) {}

bool GatewayApplication::begin() {
  esp_chip_info_t chipInfo{};
  esp_chip_info(&chipInfo);

  Serial.println(F("EcoFlow DELTA 3 bridge — milestone C (BLE + ESP-NOW TX)"));
  Serial.printf("Chip model: %s  cores=%u  revision=%u\n", ESP.getChipModel(), chipInfo.cores,
                chipInfo.revision);
  Serial.printf("Flash: %u KB  Free heap: %u\n", ESP.getFlashChipSize() / 1024U,
                ESP.getFreeHeap());
  Serial.printf("Config present: ble=%s serial=%s userId=%s peer=%s\n",
                (appConfig_.ecoflowBleAddress && appConfig_.ecoflowBleAddress[0]) ? "yes" : "no",
                (appConfig_.ecoflowSerial && appConfig_.ecoflowSerial[0]) ? "yes" : "no",
                (appConfig_.ecoflowUserId && appConfig_.ecoflowUserId[0]) ? "yes" : "no",
                (appConfig_.espNowPeerMac && appConfig_.espNowPeerMac[0]) ? "yes" : "no");

  espHealth_.begin();

  if (appConfig_.espNowPeerMac != nullptr && appConfig_.espNowPeerMac[0] != '\0') {
    if (!espNowPublisher_.begin(appConfig_.wifiSsid, appConfig_.espNowPeerMac,
                                appConfig_.espNowChannel, appConfig_.espNowPmk)) {
      Serial.printf("ESP-NOW begin failed: %s\n", espNowPublisher_.lastError());
    }
  } else {
    Serial.println(F("ESP-NOW skipped (set ESPNOW_PEER_MAC)"));
  }

  const bool bleOk =
      bleClient_.begin(appConfig_.ecoflowBleAddress, appConfig_.ecoflowSerial,
                       appConfig_.ecoflowUserId);
  if (!bleOk) {
    Serial.printf("EcoFlow BLE begin failed: %s\n", bleClient_.lastError());
  }
  lastHeartbeatMs_ = millis();
  lastPublishMs_ = millis();
  return true;
}

void GatewayApplication::printHeartbeat(uint32_t nowMs) {
  ++heartbeatCount_;
  const auto& t = bleClient_.telemetry();
  const auto health = espHealth_.snapshot();
  Serial.printf(
      "[hb %lu] up=%lus heap=%u cpu0=%u%% chip=%.0fC ble=%s auth=%s soc=%u acOut=%dW "
      "notify=%lu tel=%lu err='%s'\n",
      static_cast<unsigned long>(heartbeatCount_), static_cast<unsigned long>(nowMs / 1000U),
      ESP.getFreeHeap(), static_cast<unsigned>(health.cpuCore0Percent),
      static_cast<double>(health.chipTemperatureC), bleClient_.isConnected() ? "up" : "down",
      bleClient_.isAuthenticated() ? "ok" : "--", static_cast<unsigned>(t.socPercent),
      static_cast<int>(t.acOutputW), static_cast<unsigned long>(bleClient_.notifyCount()),
      static_cast<unsigned long>(bleClient_.telemetryCount()), bleClient_.lastError());
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
  espNowPublisher_.publish(bleClient_.telemetry(), bleClient_.isConnected(),
                           bleClient_.telemetryCount(), bleClient_.lastError(),
                           espHealth_.snapshot());
}

void GatewayApplication::loop() {
  bleClient_.loop();
  espHealth_.sample();
  const uint32_t nowMs = millis();
  maybePublishEspNow(nowMs);
  if (lastHeartbeatMs_ == 0 ||
      (nowMs - lastHeartbeatMs_) >= config::TimingConstants::kHeartbeatMs) {
    lastHeartbeatMs_ = nowMs;
    printHeartbeat(nowMs);
  }
  delay(config::TimingConstants::kAppLoopDelayMs);
}

}  // namespace ecoflow::composition
