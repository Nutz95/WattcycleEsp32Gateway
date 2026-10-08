#include "Auth/AuthService.h"
#include "Auth/NvsCredentialStore.h"
#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfig.h"
#include "Display/M5StatusDisplay.h"
#include "EspNow/EspNowTelemetryReceiver.h"
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
  Serial.println(F("Wattcycle ESP32 Gateway — M5 hub"));
  Serial.println(F("Board: M5Stack Basic (ESP32-D0WDQ6)"));
  Serial.println(F("Mode: Wi-Fi web + ESP-NOW multi-peer RX"));

  const wattcycle::config::AppConfig config =
      wattcycle::config::AppConfigFactory::fromBuildFlags();

  Serial.printf("BMS bridge MAC: %s\n",
                config.espNowBmsBridgeMac[0] ? config.espNowBmsBridgeMac : "(unset)");
  Serial.printf("XT bridge MAC: %s\n",
                config.espNowXtBridgeMac[0] ? config.espNowXtBridgeMac : "(unset)");

  static wattcycle::wifi::EspWifiConnector wifiConnector;
  static wattcycle::ota::ArduinoOtaUpdater otaUpdater;
  static wattcycle::telemetry::InMemoryTelemetryStore telemetryStore;
  static wattcycle::espnow_rx::EspNowTelemetryReceiver espNowReceiver(telemetryStore);
  static wattcycle::auth::NvsCredentialStore credentialStore;
  static wattcycle::auth::AuthService authService(credentialStore);
  authService.begin();
  static wattcycle::web::EspWebGateway webGateway(telemetryStore, authService, espNowReceiver);
  static wattcycle::display::M5StatusDisplay statusDisplay;

  static wattcycle::composition::GatewayApplication application(
      config, wifiConnector, otaUpdater, telemetryStore, webGateway, statusDisplay, authService,
      espNowReceiver);
  gApplication = &application;

  if (!application.begin()) {
    Serial.println(F("Hub boot incomplete — check Serial / display errors"));
  }
}

void loop() {
  if (gApplication != nullptr) {
    gApplication->loop();
  }
}
