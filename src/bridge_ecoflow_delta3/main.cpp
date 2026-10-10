#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfigFactory.h"
#include "EcoFlow/Ble/EcoFlowBleClient.h"
#include "Esp/EspHealthSampler.h"
#include "EspNow/EspNowTelemetryPublisher.h"

#include <Arduino.h>

#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

namespace {

ecoflow::composition::GatewayApplication* gApplication = nullptr;

}  // namespace

void setup() {
  // Shared USB cut → all bridges reboot at once. Let the rail recover before
  // any radio init, then disable the brownout detector so a brief dip cannot
  // reboot-loop (which drops EcoFlow telemetry until a manual power cycle).
  delay(1500);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("[ecoflow] console up (COM8 native USB)"));
  Serial.println(F("[ecoflow] brownout detector off (cold-boot / shared USB)"));
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
