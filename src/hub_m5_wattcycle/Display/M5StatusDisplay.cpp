#include "Display/M5StatusDisplay.h"

#include "Display/DisplayPins.h"

#include <cstdio>

#ifndef UNIT_TEST
#include <M5Unified.h>
#endif

namespace wattcycle::display {

void M5StatusDisplay::begin() {
#ifndef UNIT_TEST
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  ready_ = true;
  setBacklight(true);
#else
  ready_ = true;
#endif
}

void M5StatusDisplay::setBacklight(bool on) {
  backlightOn_ = on;
#ifndef UNIT_TEST
  M5.Display.setBrightness(on ? 80 : 0);
#endif
}

bool M5StatusDisplay::isBacklightOn() const {
  return backlightOn_;
}

uint8_t M5StatusDisplay::pageIndex() const {
  return pageIndex_;
}

void M5StatusDisplay::nextPage() {
  pageIndex_ = static_cast<uint8_t>((pageIndex_ + 1) % kPageCount);
}

void M5StatusDisplay::previousPage() {
  pageIndex_ = static_cast<uint8_t>((pageIndex_ + kPageCount - 1) % kPageCount);
}

void M5StatusDisplay::drawOverview(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto battery = store.battery();
  const auto solar = store.solar();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Display.setCursor(8, 8);
  M5.Display.printf("Hub M5  p%u", pageIndex_);
  M5.Display.setCursor(8, 40);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  if (battery.valid) {
    M5.Display.printf("BMS %u%%  %.1fV  %.1fA", battery.stateOfChargePercent,
                      static_cast<double>(battery.moduleVoltage),
                      static_cast<double>(battery.currentAmps));
  } else {
    M5.Display.print("BMS --");
  }
  M5.Display.setCursor(8, 72);
  if (solar.linkFresh) {
    M5.Display.printf("XT %.1fV  %.2fA  %.1fWh", static_cast<double>(solar.voltageV),
                      static_cast<double>(solar.currentA), static_cast<double>(solar.energyWh));
  } else {
    M5.Display.print("XT --");
  }
#else
  (void)store;
#endif
}

void M5StatusDisplay::drawGateway(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto gateway = store.status();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.setCursor(8, 8);
  M5.Display.print("Gateway");
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(8, 40);
  M5.Display.printf("WiFi %s", gateway.wifiConnected ? "OK" : "DOWN");
  M5.Display.setCursor(8, 72);
  M5.Display.printf("%s:%u", gateway.wifiIp, gateway.webPort);
  M5.Display.setCursor(8, 104);
  M5.Display.printf("BLE bridge %s", gateway.bleConnected ? "OK" : "DOWN");
#else
  (void)store;
#endif
}

void M5StatusDisplay::render(const telemetry::ITelemetryStore& store) {
  if (!ready_) {
    return;
  }
  if (pageIndex_ == 0) {
    drawOverview(store);
  } else {
    drawGateway(store);
  }
}

void M5StatusDisplay::renderAuthPrompt(const auth::AuthPrompt& prompt) {
#ifndef UNIT_TEST
  if (!ready_ || prompt.kind == auth::AuthPromptKind::None) {
    return;
  }
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Display.setCursor(8, 8);
  if (prompt.kind == auth::AuthPromptKind::ConfirmSetup) {
    M5.Display.print("Confirm web login");
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(8, 48);
    M5.Display.printf("User: %s", prompt.username);
    M5.Display.setCursor(8, 88);
    M5.Display.print("B=OK  A/C=Cancel");
  } else {
    M5.Display.print("Reset web password?");
    M5.Display.setCursor(8, 88);
    M5.Display.print("B=OK  A/C=Cancel");
  }
#else
  (void)prompt;
#endif
}

}  // namespace wattcycle::display
