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

void bleLoopYield() {
  if (gApplication != nullptr) {
    gApplication->serviceNetwork();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("Wattcycle ESP32 Gateway"));
  Serial.println(F("Board: ESP32-D0WDQ6 TTGO T-Display"));

  const wattcycle::config::AppConfig config =
      wattcycle::config::AppConfigFactory::fromBuildFlags();

  static wattcycle::wifi::EspWifiConnector wifiConnector;
  static wattcycle::ota::ArduinoOtaUpdater otaUpdater;
  static wattcycle::bms::WattcycleBleClient bleClient;
  static wattcycle::telemetry::InMemoryTelemetryStore telemetryStore;
  static wattcycle::web::EspWebGateway webGateway(telemetryStore);
  static wattcycle::display::TtgoStatusDisplay statusDisplay;

  wattcycle::bms::WattcycleBleClient::setLoopYield(bleLoopYield);

  static wattcycle::composition::GatewayApplication application(
      config, wifiConnector, otaUpdater, bleClient, telemetryStore, webGateway,
      statusDisplay);
  gApplication = &application;

  if (!application.begin()) {
    Serial.println(F("Gateway boot incomplete — check Serial / display errors"));
  } else {
    Serial.printf("Web UI: http://<device-ip>:%u/\n", config.webServerPort);
  }
}

void loop() {
  if (gApplication != nullptr) {
    gApplication->loop();
  }
}
