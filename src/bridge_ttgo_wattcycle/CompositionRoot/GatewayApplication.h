#pragma once

#include "Bms/Ble/IBmsBleClient.h"
#include "Config/AppConfig.h"
#include "Display/IButtonSource.h"
#include "Display/IStatusDisplay.h"
#include "Esp/EspHealthSampler.h"
#include "EspNow/IEspNowTelemetryPublisher.h"
#include "Telemetry/ITelemetryStore.h"
#include "Telemetry/TelemetryPoller.h"
#include "boards/common/AppCoreDisplayUiTask.h"
#include "boards/common/IUiFrameSource.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace wattcycle::composition {

class GatewayApplication : public boards::ui::IUiFrameSource {
 public:
  GatewayApplication(const config::AppConfig& appConfig, bms::IBmsBleClient& bleClient,
                     telemetry::ITelemetryStore& telemetryStore,
                     espnow_tx::IEspNowTelemetryPublisher& espNowPublisher,
                     display::IStatusDisplay& statusDisplay,
                     display::IButtonSource& buttonSource);

  bool begin();
  void loop();

  /// IUiFrameSource — panel sleep flag for the APP_CPU UI pump.
  bool isDisplayAsleep() const override;
  /// IUiFrameSource — fingerprint-gated render on APP_CPU.
  void refreshIfDirty() override;

 private:
  void startBleTask();
  void maybePublishEspNow(uint32_t nowMs);
  void sampleEspHealth();
  void handleButtons(uint32_t nowMs);
  void notifyDisplay();
  void sleepDisplay();
  void wakeDisplay(uint32_t nowMs);
  void maybeSleepDisplay(uint32_t nowMs);
  void renderCurrentDisplay();
  uint32_t displayFingerprint() const;

  static void bleTaskTrampoline(void* context);
  void bleTaskLoop();

  config::AppConfig appConfig_;
  telemetry::ITelemetryStore& telemetryStore_;
  telemetry::TelemetryPoller telemetryPoller_;
  espnow_tx::IEspNowTelemetryPublisher& espNowPublisher_;
  display::IStatusDisplay& statusDisplay_;
  display::IButtonSource& buttonSource_;
  esp_sys::EspHealthSampler espHealthSampler_;
  boards::ui::AppCoreDisplayUiTask displayUi_;
  uint32_t lastInputMs_ = 0;
  uint32_t lastDisplayFingerprint_ = 0;
  uint32_t lastEspNowPublishMs_ = 0;
  volatile bool displayAsleep_ = false;
  TaskHandle_t bleTaskHandle_ = nullptr;
};

}  // namespace wattcycle::composition
