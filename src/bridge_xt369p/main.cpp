#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfig.h"
#include "Config/TimingConstants.h"
#include "Display/TtgoStatusDisplay.h"
#include "EspNow/EspNowTelemetryPublisher.h"
#include "Meter/Spp/AtorchSppClient.h"
#include "Telemetry/InMemoryTelemetryStore.h"

#include <Arduino.h>
#include <esp_bt.h>

#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

namespace {

xt369p::composition::GatewayApplication* gApplication = nullptr;

}  // namespace

void setup() {
  // Shared USB hub / power cut: settle the rail before Wi-Fi + Classic BT.
  // BOD off avoids reboot-loops on brief dips; radio still needs a stiff enough
  // supply for inquiry — PC USB usually works, cheap hubs often do not.
  delay(xt369p::config::TimingConstants::kColdBootSettleMs);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("XT369P SPP → ESP-NOW Bridge"));
  Serial.println(F("Board: ESP32-D0WDQ6 TTGO T-Display (dual-core)"));
  Serial.println(F("Radio: Classic SPP + ESP-NOW TX (no local web)"));

  const esp_err_t bleRelease = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
  Serial.printf("BLE mem release: %s (%d)\n", esp_err_to_name(bleRelease),
                static_cast<int>(bleRelease));
  Serial.printf("Free heap boot: %u\n", ESP.getFreeHeap());

  const xt369p::config::AppConfig config =
      xt369p::config::AppConfigFactory::fromBuildFlags();

  Serial.printf("SPP target: %s\n",
                (config.xt369pBtAddress && config.xt369pBtAddress[0])
                    ? config.xt369pBtAddress
                    : config.sppDeviceName);
  Serial.printf("ESP-NOW peer: %s\n",
                config.espNowPeerMac[0] ? config.espNowPeerMac : "(unset)");

  static xt369p::meter::AtorchSppClient sppClient;
  static xt369p::telemetry::InMemoryTelemetryStore telemetryStore;
  static xt369p::espnow_tx::EspNowTelemetryPublisher espNowPublisher;
  static xt369p::display::TtgoStatusDisplay statusDisplay;

  static xt369p::composition::GatewayApplication application(
      config, sppClient, telemetryStore, espNowPublisher, statusDisplay);
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
