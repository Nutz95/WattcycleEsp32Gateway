#pragma once

#include "Auth/IAuthPhysicalConfirm.h"
#include "Bms/Ble/IBmsBleClient.h"
#include "Config/AppConfig.h"
#include "Display/ButtonNavigator.h"
#include "Display/IStatusDisplay.h"
#include "Esp/EspHealthSampler.h"
#include "EspNow/IEspNowTelemetryReceiver.h"
#include "Ota/IOtaUpdater.h"
#include "Telemetry/ITelemetryStore.h"
#include "Telemetry/TelemetryPoller.h"
#include "Web/IWebGateway.h"
#include "Wifi/IWifiConnector.h"

#ifndef UNIT_TEST
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace wattcycle::composition {

class GatewayApplication {
 public:
  GatewayApplication(const config::AppConfig& appConfig, wifi::IWifiConnector& wifiConnector,
                     ota::IOtaUpdater& otaUpdater, bms::IBmsBleClient& bleClient,
                     telemetry::ITelemetryStore& telemetryStore, web::IWebGateway& webGateway,
                     display::IStatusDisplay& statusDisplay,
                     auth::IAuthPhysicalConfirm& authConfirm,
                     espnow_rx::IEspNowTelemetryReceiver& espNowReceiver);

  bool begin();
  void loop();
  void serviceNetwork();

 private:
  void startBleTask();
  void startDisplayTask();
  void maybeStartEspNow(uint32_t nowMs);
  void refreshWifiStatus(uint32_t nowMs);
  void sampleEspHealth();
  void handleButtons(uint32_t nowMs);
  bool handleAuthUi(const display::ButtonEvent& event, uint32_t nowMs);
  bool authUiActive() const;
  void notifyDisplay();
  void sleepDisplay();
  void wakeDisplay(uint32_t nowMs);
  void maybeSleepDisplay(uint32_t nowMs);
  void renderCurrentDisplay();
  uint32_t displayFingerprint() const;

#ifndef UNIT_TEST
  static void bleTaskTrampoline(void* context);
  void bleTaskLoop();
  static void displayTaskTrampoline(void* context);
  void displayTaskLoop();
#endif

  config::AppConfig appConfig_;
  wifi::IWifiConnector& wifiConnector_;
  ota::IOtaUpdater& otaUpdater_;
  bms::IBmsBleClient& bleClient_;
  telemetry::ITelemetryStore& telemetryStore_;
  telemetry::TelemetryPoller telemetryPoller_;
  web::IWebGateway& webGateway_;
  display::IStatusDisplay& statusDisplay_;
  auth::IAuthPhysicalConfirm& authConfirm_;
  espnow_rx::IEspNowTelemetryReceiver& espNowReceiver_;
  display::ButtonNavigator buttonNavigator_;
  esp_sys::EspHealthSampler espHealthSampler_;
  uint32_t lastWifiStatusMs_ = 0;
  uint32_t lastInputMs_ = 0;
  uint32_t lastDisplayFingerprint_ = 0;
  uint32_t bootMs_ = 0;
  bool espNowAttempted_ = false;
  volatile bool displayAsleep_ = false;
#ifndef UNIT_TEST
  TaskHandle_t bleTaskHandle_ = nullptr;
  TaskHandle_t displayTaskHandle_ = nullptr;
#endif
};

}  // namespace wattcycle::composition
