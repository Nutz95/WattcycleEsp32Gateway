#include "Auth/AuthService.h"
#include "Auth/NvsCredentialStore.h"
#include "Bms/Ble/WattcycleBleClient.h"
#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfig.h"
#include "Display/TtgoStatusDisplay.h"
#include "Ota/ArduinoOtaUpdater.h"
#include "Telemetry/InMemoryTelemetryStore.h"
#include "Web/EspWebGateway.h"
#include "Wifi/EspWifiConnector.h"

#include <Arduino.h>

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
  static wattcycle::auth::NvsCredentialStore credentialStore;
  static wattcycle::auth::AuthService authService(credentialStore);
  authService.begin();
  static wattcycle::web::EspWebGateway webGateway(telemetryStore, authService);
  static wattcycle::display::TtgoStatusDisplay statusDisplay;

  static wattcycle::composition::GatewayApplication application(
      config, wifiConnector, otaUpdater, bleClient, telemetryStore, webGateway,
      statusDisplay, authService);
  gApplication = &application;

  if (!application.begin()) {
    Serial.println(F("Gateway boot incomplete — check Serial / display errors"));
  }
}

void loop() {
  if (gApplication != nullptr) {
    gApplication->loop();
  }
}
