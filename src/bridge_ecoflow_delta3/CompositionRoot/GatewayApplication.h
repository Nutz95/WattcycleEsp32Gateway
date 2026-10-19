#pragma once

#include "Config/AppConfig.h"
#include "Display/IStatusDisplay.h"
#include "EcoFlow/Ble/IEcoFlowBleClient.h"
#include "Esp/EspHealthSampler.h"
#include "EspNow/IEspNowTelemetryPublisher.h"
#include "boards/common/IButtonSource.h"
#include "boards/common/AppCoreDisplayUiTask.h"
#include "boards/common/IUiFrameSource.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace ecoflow::composition {

/// Owns BLE + ESP-NOW (+ optional local display) for the DELTA 3 bridge.
class GatewayApplication : public boards::ui::IUiFrameSource {
 public:
  /// Wire BLE, ESP-NOW, health, display, and button collaborators.
  GatewayApplication(const config::AppConfig& appConfig, ble::IEcoFlowBleClient& bleClient,
                     espnow_tx::IEspNowTelemetryPublisher& espNowPublisher,
                     esp_sys::EspHealthSampler& espHealth, display::IStatusDisplay& statusDisplay,
                     boards::input::IButtonSource& buttonSource);

  /// Start display, ESP-NOW (if peer MAC set), then BLE + UI tasks.
  bool begin();
  /// Publish ESP-NOW + heartbeat + buttons (BLE/UI on pinned tasks).
  void loop();

  /// IUiFrameSource — panel sleep flag for the APP_CPU UI pump.
  bool isDisplayAsleep() const override;
  /// IUiFrameSource — fingerprint-gated render on APP_CPU.
  void refreshIfDirty() override;

 private:
  void printHeartbeat(uint32_t nowMs);
  void maybePublishEspNow(uint32_t nowMs);
  void handleButtons(uint32_t nowMs);
  void maybeSleepDisplay(uint32_t nowMs);
  void sleepDisplay();
  void wakeDisplay(uint32_t nowMs);
  void renderCurrentDisplay();
  void notifyDisplay();
  uint32_t displayFingerprint() const;
  void startBleTask();
  static void bleTaskTrampoline(void* context);
  void bleTaskLoop();

  config::AppConfig appConfig_;
  ble::IEcoFlowBleClient& bleClient_;
  espnow_tx::IEspNowTelemetryPublisher& espNowPublisher_;
  esp_sys::EspHealthSampler& espHealth_;
  display::IStatusDisplay& statusDisplay_;
  boards::input::IButtonSource& buttonSource_;
  boards::ui::AppCoreDisplayUiTask displayUi_;
  uint32_t lastHeartbeatMs_ = 0;
  uint32_t lastPublishMs_ = 0;
  uint32_t lastDisplayFingerprint_ = 0;
  uint32_t lastInputMs_ = 0;
  uint32_t heartbeatCount_ = 0;
  volatile bool displayAsleep_ = false;
  TaskHandle_t bleTaskHandle_ = nullptr;
};

}  // namespace ecoflow::composition
