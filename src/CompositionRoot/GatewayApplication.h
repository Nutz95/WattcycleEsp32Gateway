#pragma once

#include "Bms/Ble/IBmsBleClient.h"
#include "Config/AppConfig.h"
#include "Display/IStatusDisplay.h"
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

/// Owns the application loop; depends only on interfaces (wired in main).
class GatewayApplication {
 public:
  GatewayApplication(const config::AppConfig& appConfig, wifi::IWifiConnector& wifiConnector,
                     ota::IOtaUpdater& otaUpdater, bms::IBmsBleClient& bleClient,
                     telemetry::ITelemetryStore& telemetryStore, web::IWebGateway& webGateway,
                     display::IStatusDisplay& statusDisplay);

  bool begin();
  void loop();
  void serviceNetwork();

 private:
  void startBleTask();
  void refreshWifiStatus(uint32_t nowMs);
  void maybeRefreshDisplay(uint32_t nowMs);

#ifndef UNIT_TEST
  static void bleTaskTrampoline(void* context);
  void bleTaskLoop();
#endif

  config::AppConfig appConfig_;
  wifi::IWifiConnector& wifiConnector_;
  ota::IOtaUpdater& otaUpdater_;
  bms::IBmsBleClient& bleClient_;
  telemetry::ITelemetryStore& telemetryStore_;
  telemetry::TelemetryPoller telemetryPoller_;
  web::IWebGateway& webGateway_;
  display::IStatusDisplay& statusDisplay_;
  uint32_t lastDisplayMs_ = 0;
  uint32_t lastWifiStatusMs_ = 0;
#ifndef UNIT_TEST
  TaskHandle_t bleTaskHandle_ = nullptr;
#endif
};

}  // namespace wattcycle::composition
