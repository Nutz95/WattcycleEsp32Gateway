#include "Display/TtgoStatusDisplay.h"

#ifndef UNIT_TEST

#include <TFT_eSPI.h>

namespace wattcycle::display {
namespace {

TFT_eSPI tft;

}  // namespace

void TtgoStatusDisplay::begin() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  ready_ = true;
}

void TtgoStatusDisplay::render(const telemetry::ITelemetryStore& store) {
  if (!ready_) {
    return;
  }

  const auto battery = store.battery();
  const auto gateway = store.status();

  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Wattcycle GW", 4, 6);

  tft.setTextSize(1);
  tft.setCursor(4, 36);
  tft.printf("WiFi: %s", gateway.wifiConnected ? gateway.wifiIp : "down");
  tft.setCursor(4, 52);
  tft.printf("BLE : %s", gateway.bleConnected ? "ok" : "down");

  if (battery.valid) {
    tft.setTextSize(3);
    tft.setCursor(4, 78);
    tft.printf("%u%%", battery.stateOfChargePercent);
    tft.setTextSize(1);
    tft.setCursor(4, 112);
    tft.printf("%.1fV  %.1fA  %.0fW", battery.moduleVoltage, battery.currentAmps,
               battery.powerWatts);
  } else {
    tft.setTextSize(1);
    tft.setCursor(4, 78);
    tft.print("Waiting for BMS...");
    if (gateway.lastError[0] != '\0') {
      tft.setCursor(4, 96);
      tft.print(gateway.lastError);
    }
  }
}

}  // namespace wattcycle::display

#endif
