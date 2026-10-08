#include "Auth/AuthService.h"
#include "Auth/NvsCredentialStore.h"
#include "CompositionRoot/GatewayApplication.h"
#include "Config/AppConfig.h"
#include "Display/M5StatusDisplay.h"
#include "EspNow/EspNowTelemetryReceiver.h"
#include "Ota/ArduinoOtaUpdater.h"
#include "Storage/SdDailyHistory.h"
#include "Telemetry/InMemoryTelemetryStore.h"
#include "Time/NtpClock.h"
#include "HttpApi/EspWebGateway.h"
#include "Wifi/EspWifiConnector.h"

#include <Arduino.h>
#include <WebServer.h>  // force PlatformIO LDF (chain+) to link framework WebServer
#include <WiFi.h>

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
  static wattcycle::time_sync::NtpClock ntpClock;
  static wattcycle::storage::SdDailyHistory sdHistory;
  static wattcycle::web::EspWebGateway webGateway(telemetryStore, authService, espNowReceiver,
                                                  sdHistory);
  static wattcycle::display::M5StatusDisplay statusDisplay;

  // STA MAC is known after Wi-Fi mode init inside connect(); seed peers from build flags now.
  webGateway.setDeviceIdentity("m5_hub", "", config.espNowBmsBridgeMac, config.espNowXtBridgeMac);

  static wattcycle::composition::GatewayApplication application(
      config, wifiConnector, otaUpdater, telemetryStore, webGateway, statusDisplay, authService,
      espNowReceiver, ntpClock, sdHistory);
  gApplication = &application;

  if (!application.begin()) {
    Serial.println(F("Hub boot incomplete — check Serial / display errors"));
  }

  webGateway.setDeviceIdentity("m5_hub", WiFi.macAddress().c_str(), config.espNowBmsBridgeMac,
                               config.espNowXtBridgeMac);
}

void loop() {
  if (gApplication != nullptr) {
    gApplication->loop();
  }
}
