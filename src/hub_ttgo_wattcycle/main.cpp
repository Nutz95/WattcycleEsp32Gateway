#include "Auth/AuthService.h"
#include "Auth/NvsCredentialStore.h"
#include "Bms/Ble/WattcycleBleClient.h"
#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfig.h"
#include "Display/TtgoStatusDisplay.h"
#include "EspNow/EspNowTelemetryReceiver.h"
#include "Ota/ArduinoOtaUpdater.h"
#include "Storage/NullDailyHistoryStore.h"
#include "Telemetry/InMemoryTelemetryStore.h"
#include "HttpApi/EspWebGateway.h"
#include "Wifi/EspWifiConnector.h"

#include <Arduino.h>
#include <WebServer.h>  // force LDF to link framework WebServer
#include <WiFi.h>

namespace {

wattcycle::composition::GatewayApplication* gApplication = nullptr;

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("Wattcycle ESP32 Gateway"));
  Serial.println(F("Board: ESP32-D0WDQ6 TTGO T-Display (dual-core)"));
  Serial.println(F("Mode: monitoring only (no BMS writes)"));

  const wattcycle::config::AppConfig config =
      wattcycle::config::AppConfigFactory::fromBuildFlags();

  Serial.printf("BMS target: %s\n",
                config.bmsBleAddress[0] ? config.bmsBleAddress : "(unset)");

  static wattcycle::wifi::EspWifiConnector wifiConnector;
  static wattcycle::ota::ArduinoOtaUpdater otaUpdater;
  static wattcycle::bms::WattcycleBleClient bleClient;
  static wattcycle::telemetry::InMemoryTelemetryStore telemetryStore;
  static wattcycle::espnow_rx::EspNowTelemetryReceiver espNowReceiver(telemetryStore);
  static wattcycle::auth::NvsCredentialStore credentialStore;
  static wattcycle::auth::AuthService authService(credentialStore);
  authService.begin();
  static wattcycle::storage::NullDailyHistoryStore nullHistory;
  static wattcycle::web::EspWebGateway webGateway(telemetryStore, authService, espNowReceiver,
                                                  nullHistory);
  static wattcycle::display::TtgoStatusDisplay statusDisplay;
  webGateway.setDeviceIdentity("ttgo_hub", "", "", "");

  static wattcycle::composition::GatewayApplication application(
      config, wifiConnector, otaUpdater, bleClient, telemetryStore, webGateway,
      statusDisplay, authService, espNowReceiver);
  gApplication = &application;

  if (!application.begin()) {
    Serial.println(F("Gateway boot incomplete — check Serial / display errors"));
  }
  webGateway.setDeviceIdentity("ttgo_hub", WiFi.macAddress().c_str(), "",
                               config.espNowBridgeMac);
}

void loop() {
  if (gApplication != nullptr) {
    gApplication->loop();
  }
}
