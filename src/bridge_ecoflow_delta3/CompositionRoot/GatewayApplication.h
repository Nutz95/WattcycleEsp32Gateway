#pragma once

#include "Config/AppConfig.h"
#include "EcoFlow/Ble/IEcoFlowBleClient.h"
#include "Esp/EspHealthSampler.h"
#include "EspNow/IEspNowTelemetryPublisher.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace ecoflow::composition {

/// Owns the BLE + ESP-NOW loop for the headless DELTA 3 bridge.
class GatewayApplication {
 public:
  GatewayApplication(const config::AppConfig& appConfig, ble::IEcoFlowBleClient& bleClient,
                     espnow_tx::IEspNowTelemetryPublisher& espNowPublisher,
                     esp_sys::EspHealthSampler& espHealth);

  /// Start ESP-NOW (if peer MAC set) then BLE client.
  bool begin();
  /// Publish ESP-NOW + heartbeat from mutex snapshots; BLE runs on its own task.
  void loop();

 private:
  void printHeartbeat(uint32_t nowMs);
  void maybePublishEspNow(uint32_t nowMs);
  void startBleTask();
  static void bleTaskTrampoline(void* context);
  void bleTaskLoop();

  config::AppConfig appConfig_;
  ble::IEcoFlowBleClient& bleClient_;
  espnow_tx::IEspNowTelemetryPublisher& espNowPublisher_;
  esp_sys::EspHealthSampler& espHealth_;
  uint32_t lastHeartbeatMs_ = 0;
  uint32_t lastPublishMs_ = 0;
  uint32_t heartbeatCount_ = 0;
  TaskHandle_t bleTaskHandle_ = nullptr;
};

}  // namespace ecoflow::composition
