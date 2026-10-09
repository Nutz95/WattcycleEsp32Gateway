#include "CompositionRoot/GatewayApplication.h"

#include "Config/TimingConstants.h"

#include <Arduino.h>
#include <esp_chip_info.h>

namespace ecoflow::composition {

GatewayApplication::GatewayApplication(const config::AppConfig& appConfig)
    : appConfig_(appConfig) {}

bool GatewayApplication::begin() {
  esp_chip_info_t chipInfo{};
  esp_chip_info(&chipInfo);

  Serial.println(F("EcoFlow DELTA 3 bridge — scaffold (USB heartbeat)"));
  Serial.printf("Chip model: %s  cores=%u  revision=%u\n", ESP.getChipModel(), chipInfo.cores,
                chipInfo.revision);
  Serial.printf("Flash: %u KB  Free heap: %u\n", ESP.getFlashChipSize() / 1024U,
                ESP.getFreeHeap());
  Serial.printf("Config present: ble=%s serial=%s userId=%s peer=%s\n",
                (appConfig_.ecoflowBleAddress && appConfig_.ecoflowBleAddress[0]) ? "yes" : "no",
                (appConfig_.ecoflowSerial && appConfig_.ecoflowSerial[0]) ? "yes" : "no",
                (appConfig_.ecoflowUserId && appConfig_.ecoflowUserId[0]) ? "yes" : "no",
                (appConfig_.espNowPeerMac && appConfig_.espNowPeerMac[0]) ? "yes" : "no");
  Serial.println(F("Milestone A: no NimBLE / no EcoFlow crypto / no ESP-NOW yet."));
  lastHeartbeatMs_ = millis();
  return true;
}

void GatewayApplication::printHeartbeat(uint32_t nowMs) {
  ++heartbeatCount_;
  Serial.printf("[hb %lu] uptime=%lus heap=%u\n", static_cast<unsigned long>(heartbeatCount_),
                static_cast<unsigned long>(nowMs / 1000U), ESP.getFreeHeap());
}

void GatewayApplication::loop() {
  const uint32_t nowMs = millis();
  if (lastHeartbeatMs_ == 0 ||
      (nowMs - lastHeartbeatMs_) >= config::TimingConstants::kHeartbeatMs) {
    lastHeartbeatMs_ = nowMs;
    printHeartbeat(nowMs);
  }
  delay(config::TimingConstants::kAppLoopDelayMs);
}

}  // namespace ecoflow::composition
