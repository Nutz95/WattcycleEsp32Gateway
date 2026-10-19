#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfigFactory.h"
#include "Display/DongleStatusDisplay.h"
#include "EcoFlow/Ble/EcoFlowBleClient.h"
#include "Esp/EspHealthSampler.h"
#include "EspNow/EspNowTelemetryPublisher.h"
#include "boards/lilygo_t_dongle_s3/DongleButtonSource.h"

#include <Arduino.h>

#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

namespace {

ecoflow::composition::GatewayApplication* gApplication = nullptr;

}  // namespace

void setup() {
  delay(1500);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("[ecoflow] console up (T-Dongle-S3)"));
  Serial.println(F("[ecoflow] brownout detector off (cold-boot / shared USB)"));
  Serial.printf("[ecoflow] NimBLE MSYS1_BLOCK_COUNT=%d\n",
                CONFIG_BT_NIMBLE_MSYS1_BLOCK_COUNT);

  const ecoflow::config::AppConfig config = ecoflow::config::AppConfigFactory::fromBuildFlags();
  static ecoflow::ble::EcoFlowBleClient bleClient;
  static ecoflow::espnow_tx::EspNowTelemetryPublisher espNowPublisher;
  static ecoflow::esp_sys::EspHealthSampler espHealth;
  static ecoflow::display::DongleStatusDisplay statusDisplay;
  static boards::lilygo_t_dongle_s3::DongleButtonSource buttonSource;
  static ecoflow::composition::GatewayApplication application(
      config, bleClient, espNowPublisher, espHealth, statusDisplay, buttonSource);
  gApplication = &application;
  application.begin();
}

void loop() {
  if (gApplication != nullptr) {
    gApplication->loop();
  }
}
