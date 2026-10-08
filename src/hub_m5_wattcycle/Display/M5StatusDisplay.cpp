#include "Display/M5StatusDisplay.h"

#include "Display/DisplayPins.h"

#include <cstdio>

namespace wattcycle::display {

void M5StatusDisplay::begin() {
#ifndef UNIT_TEST
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  // 8-bit canvas: 320x240 ~= 77 KB — safer than 16-bit with Wi-Fi heap pressure.
  canvas_.setColorDepth(8);
  if (!canvas_.createSprite(kDisplayWidth, kDisplayHeight)) {
    ready_ = false;
    return;
  }
  canvas_.setTextSize(2);
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

void M5StatusDisplay::pushFrame() {
#ifndef UNIT_TEST
  canvas_.pushSprite(0, 0);
#endif
}

void M5StatusDisplay::drawOverview(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto battery = store.battery();
  const auto solar = store.solar();
  canvas_.fillSprite(TFT_BLACK);
  canvas_.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas_.setCursor(8, 8);
  canvas_.printf("Hub M5  p%u", pageIndex_);
  canvas_.setCursor(8, 40);
  canvas_.setTextColor(TFT_WHITE, TFT_BLACK);
  if (battery.valid) {
    canvas_.printf("BMS %u%%  %.1fV  %.1fA", battery.stateOfChargePercent,
                   static_cast<double>(battery.moduleVoltage),
                   static_cast<double>(battery.currentAmps));
  } else {
    canvas_.print("BMS --");
  }
  canvas_.setCursor(8, 72);
  if (solar.linkFresh) {
    canvas_.printf("XT %.1fV  %.2fA  %.1fWh", static_cast<double>(solar.voltageV),
                   static_cast<double>(solar.currentA), static_cast<double>(solar.energyWh));
  } else {
    canvas_.print("XT --");
  }
  pushFrame();
#else
  (void)store;
#endif
}

void M5StatusDisplay::drawGateway(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto gateway = store.status();
  canvas_.fillSprite(TFT_BLACK);
  canvas_.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas_.setCursor(8, 8);
  canvas_.print("Gateway");
  canvas_.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas_.setCursor(8, 40);
  canvas_.printf("WiFi %s", gateway.wifiConnected ? "OK" : "DOWN");
  canvas_.setCursor(8, 72);
  canvas_.printf("%s:%u", gateway.wifiIp, gateway.webPort);
  canvas_.setCursor(8, 104);
  canvas_.printf("BLE bridge %s", gateway.bleConnected ? "OK" : "DOWN");
  pushFrame();
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
  canvas_.fillSprite(TFT_BLACK);
  canvas_.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas_.setCursor(8, 8);
  if (prompt.kind == auth::AuthPromptKind::ConfirmSetup) {
    canvas_.print("Confirm web login");
    canvas_.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas_.setCursor(8, 48);
    canvas_.printf("User: %s", prompt.username);
    canvas_.setCursor(8, 88);
    canvas_.print("B=OK  A/C=Cancel");
  } else {
    canvas_.print("Reset web password?");
    canvas_.setCursor(8, 88);
    canvas_.print("B=OK  A/C=Cancel");
  }
  pushFrame();
#else
  (void)prompt;
#endif
}

}  // namespace wattcycle::display
