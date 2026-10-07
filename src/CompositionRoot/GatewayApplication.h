#pragma once

#include "Bms/Ble/IBmsBleClient.h"
#include "Config/AppConfig.h"
#include "Display/IStatusDisplay.h"
#include "Ota/IOtaUpdater.h"
#include "Telemetry/ITelemetryStore.h"
#include "Telemetry/TelemetryPoller.h"
#include "Web/IWebGateway.h"
#include "Wifi/IWifiConnector.h"

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
  void refreshWifiStatus();
  void maybeRefreshDisplay(uint32_t nowMs);

  config::AppConfig appConfig_;
  wifi::IWifiConnector& wifiConnector_;
  ota::IOtaUpdater& otaUpdater_;
  bms::IBmsBleClient& bleClient_;
  telemetry::ITelemetryStore& telemetryStore_;
  telemetry::TelemetryPoller telemetryPoller_;
  web::IWebGateway& webGateway_;
  display::IStatusDisplay& statusDisplay_;
  uint32_t lastDisplayMs_ = 0;
};

}  // namespace wattcycle::composition
