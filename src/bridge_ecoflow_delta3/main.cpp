#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfig.h"

#include <Arduino.h>

namespace {

ecoflow::composition::GatewayApplication* gApplication = nullptr;

}  // namespace

void setup() {
  // CDC_ON_BOOT=0 → Serial is UART0 (secondary USB-UART on DevKitC). Flash stays on native USB.
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("[ecoflow] UART console up (scaffold)"));

  const ecoflow::config::AppConfig config = ecoflow::config::AppConfigFactory::fromBuildFlags();
  static ecoflow::composition::GatewayApplication application(config);
  gApplication = &application;
  application.begin();
}

void loop() {
  if (gApplication != nullptr) {
    gApplication->loop();
  }
}
