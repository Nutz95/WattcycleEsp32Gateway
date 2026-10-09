#include "Display/M5StatusDisplay.h"

#include "Bus/SharedSpiLock.h"
#include "Display/DisplayPins.h"
#include "Display/M5DisplayPages.h"
#include "Display/M5DisplayTiles.h"

namespace wattcycle::display {

void M5StatusDisplay::begin() {
#ifndef UNIT_TEST
  bus::SharedSpiLock::begin();
  auto boardConfiguration = M5.config();
  boardConfiguration.internal_imu = false;
  boardConfiguration.internal_rtc = false;
  boardConfiguration.internal_spk = false;
  boardConfiguration.internal_mic = false;
  M5.begin(boardConfiguration);
  M5.Display.setRotation(1);
  canvas_.setColorDepth(8);
  if (!canvas_.createSprite(kDisplayWidth, kDisplayHeight)) {
    Serial.printf("M5 canvas alloc failed heap=%u\n", ESP.getFreeHeap());
    ready_ = false;
    return;
  }
  canvas_.setTextSize(2);
  ready_ = true;
  setBacklight(true);
  drawSplashFrame(canvas_);
  pushFrame();
  Serial.printf("M5 display ready heap=%u\n", ESP.getFreeHeap());
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
  bus::SpiGuard guard;
  canvas_.pushSprite(0, 0);
#endif
}

void M5StatusDisplay::render(const telemetry::ITelemetryStore& store) {
  if (!ready_) {
    return;
  }
#ifndef UNIT_TEST
  switch (pageIndex_) {
    case 0:
      drawM5OverviewPage(canvas_, store);
      drawM5ButtonFooter(canvas_, pageIndex_, kPageNames[0]);
      break;
    case 1:
      drawM5EcoflowPage(canvas_, store);
      drawM5ButtonFooter(canvas_, pageIndex_, kPageNames[1]);
      break;
    case 2:
      drawM5PackPage(canvas_, store);
      drawM5ButtonFooter(canvas_, pageIndex_, kPageNames[2]);
      break;
    case 3:
      drawM5SolarPage(canvas_, store);
      drawM5ButtonFooter(canvas_, pageIndex_, kPageNames[3]);
      break;
    case 4:
      drawM5GatewayPage(canvas_, store);
      drawM5ButtonFooter(canvas_, pageIndex_, kPageNames[4]);
      break;
    case 5:
      drawM5EspPage(canvas_, store);
      drawM5ButtonFooter(canvas_, pageIndex_, kPageNames[5]);
      break;
    case 6:
      drawM5TempsPage(canvas_, store);
      drawM5ButtonFooter(canvas_, pageIndex_, kPageNames[6]);
      break;
    default:
      pageIndex_ = 0;
      drawM5OverviewPage(canvas_, store);
      drawM5ButtonFooter(canvas_, pageIndex_, kPageNames[0]);
      break;
  }
  pushFrame();
#else
  (void)store;
#endif
}

void M5StatusDisplay::renderAuthPrompt(const auth::AuthPrompt& prompt) {
#ifndef UNIT_TEST
  if (!ready_ || prompt.kind == auth::AuthPromptKind::None) {
    return;
  }
  drawM5AuthPromptPage(canvas_, prompt);
  drawM5ButtonFooter(canvas_, pageIndex_, "OK");
  pushFrame();
#else
  (void)prompt;
#endif
}

}  // namespace wattcycle::display
