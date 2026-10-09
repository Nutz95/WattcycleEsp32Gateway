#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfigFactory.h"
#include "EcoFlow/Ble/EcoFlowBleClient.h"
#include "Esp/EspHealthSampler.h"
#include "EspNow/EspNowTelemetryPublisher.h"

#include <Arduino.h>

namespace {

ecoflow::composition::GatewayApplication* gApplication = nullptr;

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println(F("[ecoflow] console up (COM8 native USB)"));
  Serial.printf("[ecoflow] NimBLE MSYS1_BLOCK_COUNT=%d\n",
                CONFIG_BT_NIMBLE_MSYS1_BLOCK_COUNT);

  const ecoflow::config::AppConfig config = ecoflow::config::AppConfigFactory::fromBuildFlags();
  static ecoflow::ble::EcoFlowBleClient bleClient;
  static ecoflow::espnow_tx::EspNowTelemetryPublisher espNowPublisher;
  static ecoflow::esp_sys::EspHealthSampler espHealth;
  static ecoflow::composition::GatewayApplication application(config, bleClient, espNowPublisher,
                                                              espHealth);
  gApplication = &application;
  application.begin();
}

void loop() {
  if (gApplication != nullptr) {
    gApplication->loop();
  }
}
