#include "Bms/Ble/WattcycleBleClient.h"
#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfig.h"
#include "Config/TimingConstants.h"
#include "Display/DongleStatusDisplay.h"
#include "EspNow/EspNowTelemetryPublisher.h"
#include "Telemetry/InMemoryTelemetryStore.h"
#include "boards/lilygo_t_dongle_s3/DongleButtonSource.h"

#include <Arduino.h>

#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

namespace {

wattcycle::composition::GatewayApplication* gApplication = nullptr;

}  // namespace

void setup() {
  delay(wattcycle::config::TimingConstants::kColdBootSettleMs);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("Wattcycle BMS BLE → ESP-NOW Bridge"));
  Serial.println(F("Board: LILYGO T-Dongle-S3 (ST7735 80x160)"));
  Serial.println(F("Radio: NimBLE + ESP-NOW TX (no local web)"));
  Serial.printf("Free heap boot: %u\n", ESP.getFreeHeap());

  const wattcycle::config::AppConfig config =
      wattcycle::config::AppConfigFactory::fromBuildFlags();

  Serial.printf("BMS target: %s\n",
                config.bmsBleAddress[0] ? config.bmsBleAddress : "(unset)");
  Serial.printf("ESP-NOW peer: %s\n",
                config.espNowPeerMac[0] ? config.espNowPeerMac : "(unset)");

  // NimBLE 2 setPower is dBm. +9 dBm browns out the dongle USB rail with Wi-Fi up.
  static wattcycle::bms::WattcycleBleClient bleClient(3);
  static wattcycle::telemetry::InMemoryTelemetryStore telemetryStore;
  static wattcycle::espnow_tx::EspNowTelemetryPublisher espNowPublisher;
  static wattcycle::display::DongleStatusDisplay statusDisplay;
  static boards::lilygo_t_dongle_s3::DongleButtonSource buttonSource;

  static wattcycle::composition::GatewayApplication application(
      config, bleClient, telemetryStore, espNowPublisher, statusDisplay, buttonSource);
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
