#include "Bms/Ble/WattcycleBleClient.h"
#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfig.h"
#include "Display/TtgoStatusDisplay.h"
#include "EspNow/EspNowTelemetryPublisher.h"
#include "Telemetry/InMemoryTelemetryStore.h"

#include <Arduino.h>

namespace {

wattcycle::composition::GatewayApplication* gApplication = nullptr;

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("Wattcycle BMS BLE → ESP-NOW Bridge"));
  Serial.println(F("Board: ESP32-D0WDQ6 TTGO T-Display (dual-core)"));
  Serial.println(F("Radio: NimBLE + ESP-NOW TX (no local web)"));
  Serial.printf("Free heap boot: %u\n", ESP.getFreeHeap());

  const wattcycle::config::AppConfig config =
      wattcycle::config::AppConfigFactory::fromBuildFlags();

  Serial.printf("BMS target: %s\n",
                config.bmsBleAddress[0] ? config.bmsBleAddress : "(unset)");
  Serial.printf("ESP-NOW peer: %s\n",
                config.espNowPeerMac[0] ? config.espNowPeerMac : "(unset)");

  static wattcycle::bms::WattcycleBleClient bleClient;
  static wattcycle::telemetry::InMemoryTelemetryStore telemetryStore;
  static wattcycle::espnow_tx::EspNowTelemetryPublisher espNowPublisher;
  static wattcycle::display::TtgoStatusDisplay statusDisplay;

  static wattcycle::composition::GatewayApplication application(
      config, bleClient, telemetryStore, espNowPublisher, statusDisplay);
  gApplication = &application;

  if (!application.begin()) {
    Serial.println(F("Bridge boot incomplete — check Serial / display errors"));
  }
}

void loop() {
  if (gApplication != nullptr) {
    gApplication->loop();
  }
}
