#pragma once

#include "Config/AppConfig.h"
#include "Display/ButtonNavigator.h"
#include "Display/IStatusDisplay.h"
#include "Esp/EspHealthSampler.h"
#include "EspNow/IEspNowTelemetryPublisher.h"
#include "Meter/Spp/ISppMeterClient.h"
#include "Telemetry/ITelemetryStore.h"
#include "Telemetry/TelemetryPoller.h"

#ifndef UNIT_TEST
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace xt369p::composition {

class GatewayApplication {
 public:
  GatewayApplication(const config::AppConfig& appConfig, meter::ISppMeterClient& sppClient,
                     telemetry::ITelemetryStore& telemetryStore,
                     espnow_tx::IEspNowTelemetryPublisher& espNowPublisher,
                     display::IStatusDisplay& statusDisplay);

  bool begin();
  void loop();

 private:
  void startSppTask();
  void startDisplayTask();
  void maybePublishEspNow(uint32_t nowMs);
  void sampleEspHealth();
  void handleButtons(uint32_t nowMs);
  void notifyDisplay();
  void sleepDisplay();
  void wakeDisplay(uint32_t nowMs);
  void maybeSleepDisplay(uint32_t nowMs);
  void renderCurrentDisplay();
  uint32_t displayFingerprint() const;

#ifndef UNIT_TEST
  static void sppTaskTrampoline(void* context);
  void sppTaskLoop();
  static void displayTaskTrampoline(void* context);
  void displayTaskLoop();
#endif

  config::AppConfig appConfig_;
  telemetry::ITelemetryStore& telemetryStore_;
  telemetry::TelemetryPoller telemetryPoller_;
  espnow_tx::IEspNowTelemetryPublisher& espNowPublisher_;
  display::IStatusDisplay& statusDisplay_;
  display::ButtonNavigator buttonNavigator_;
  esp_sys::EspHealthSampler espHealthSampler_;
  uint32_t lastInputMs_ = 0;
  uint32_t lastDisplayFingerprint_ = 0;
  uint32_t lastEspNowPublishMs_ = 0;
  volatile bool displayAsleep_ = false;
#ifndef UNIT_TEST
  TaskHandle_t sppTaskHandle_ = nullptr;
  TaskHandle_t displayTaskHandle_ = nullptr;
#endif
};

}  // namespace xt369p::composition
