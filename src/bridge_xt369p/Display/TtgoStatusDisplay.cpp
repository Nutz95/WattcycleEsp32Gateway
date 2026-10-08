#include "Display/TtgoStatusDisplay.h"

#include "Display/DisplayPins.h"

#include <cstdio>

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

namespace xt369p::display {
namespace {

constexpr uint16_t kBg = TFT_BLACK;
constexpr uint16_t kInk = TFT_WHITE;
constexpr uint16_t kMuted = 0x8410;
constexpr uint16_t kAccent = 0x07E0;
constexpr uint16_t kDanger = 0xF800;

}  // namespace

void TtgoStatusDisplay::begin() {
#ifndef UNIT_TEST
  tft_.init();
  tft_.setRotation(1);
  tft_.fillScreen(kBg);
  pinMode(kBacklightGpio, OUTPUT);
  digitalWrite(kBacklightGpio, HIGH);
  sprite_.setColorDepth(8);
  sprite_.createSprite(240, 135);
  ready_ = true;
  backlightOn_ = true;
#endif
}

void TtgoStatusDisplay::setBacklight(bool on) {
#ifndef UNIT_TEST
  backlightOn_ = on;
  digitalWrite(kBacklightGpio, on ? HIGH : LOW);
#else
  backlightOn_ = on;
#endif
}

bool TtgoStatusDisplay::isBacklightOn() const {
  return backlightOn_;
}

void TtgoStatusDisplay::nextPage() {
  pageIndex_ = static_cast<uint8_t>((pageIndex_ + 1) % kPageCount);
}

void TtgoStatusDisplay::previousPage() {
  pageIndex_ = pageIndex_ == 0 ? static_cast<uint8_t>(kPageCount - 1)
                               : static_cast<uint8_t>(pageIndex_ - 1);
}

uint8_t TtgoStatusDisplay::pageIndex() const {
  return pageIndex_;
}

void TtgoStatusDisplay::drawChrome(const char* title) {
#ifndef UNIT_TEST
  sprite_.fillSprite(kBg);
  sprite_.setTextDatum(TL_DATUM);
  sprite_.setTextColor(kAccent, kBg);
  sprite_.drawString("XT369P", 8, 4, 2);
  sprite_.setTextColor(kInk, kBg);
  sprite_.drawString(title, 80, 4, 2);
  sprite_.drawFastHLine(8, 22, 224, kMuted);
#else
  (void)title;
#endif
}

void TtgoStatusDisplay::drawMetric(int x, int y, const char* label, const char* value) {
#ifndef UNIT_TEST
  sprite_.setTextColor(kMuted, kBg);
  sprite_.drawString(label, x, y, 1);
  sprite_.setTextColor(kInk, kBg);
  sprite_.drawString(value, x, y + 12, 2);
#else
  (void)x;
  (void)y;
  (void)label;
  (void)value;
#endif
}

void TtgoStatusDisplay::drawOverview(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  drawChrome("Solar");
  const auto m = store.meter();
  const auto g = store.status();

  sprite_.setTextColor(g.sppConnected ? kAccent : kDanger, kBg);
  sprite_.drawString(g.sppConnected ? "SPP" : "SPP!", 190, 4, 2);

  sprite_.setTextColor(g.espNowReady ? kAccent : kDanger, kBg);
  sprite_.drawString(g.espNowReady ? "NOW" : "NOW!", 150, 4, 2);

  if (!m.valid) {
    sprite_.setTextColor(kMuted, kBg);
    sprite_.drawString("Waiting for meter...", 12, 40, 2);
    if (g.lastError[0]) {
      sprite_.setTextColor(kDanger, kBg);
      sprite_.drawString(g.lastError, 12, 72, 1);
    }
    sprite_.pushSprite(0, 0);
    return;
  }

  char v[24];
  char a[24];
  char w[24];
  char e[24];
  char mah[24];
  std::snprintf(v, sizeof(v), "%.2f V", static_cast<double>(m.voltageV));
  std::snprintf(a, sizeof(a), "%.3f A", static_cast<double>(m.currentA));
  std::snprintf(w, sizeof(w), "%.2f W", static_cast<double>(m.powerW));
  std::snprintf(e, sizeof(e), "%.3f Wh", static_cast<double>(m.energyWh));
  std::snprintf(mah, sizeof(mah), "%.0f mAh", static_cast<double>(m.capacityAh * 1000.0f));

  sprite_.setTextColor(kInk, kBg);
  sprite_.drawString(v, 12, 32, 4);
  drawMetric(12, 68, "Current", a);
  drawMetric(120, 68, "Power", w);
  drawMetric(12, 98, "mAh", mah);
  drawMetric(120, 98, "Energy", e);
  sprite_.pushSprite(0, 0);
#else
  (void)store;
#endif
}

void TtgoStatusDisplay::drawGateway(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  drawChrome("Bridge");
  const auto g = store.status();

  sprite_.setTextColor(kMuted, kBg);
  sprite_.drawString("ESP-NOW peer", 12, 30, 1);
  sprite_.setTextColor(g.espNowReady ? kAccent : kDanger, kBg);
  sprite_.drawString(g.espNowPeerMac[0] ? g.espNowPeerMac : "-", 12, 44, 2);

  char chLabel[24];
  std::snprintf(chLabel, sizeof(chLabel), "ch %u", static_cast<unsigned>(g.espNowChannel));
  sprite_.setTextColor(kMuted, kBg);
  sprite_.drawString("ESP-NOW radio", 12, 72, 1);
  sprite_.setTextColor(g.espNowReady ? kAccent : kDanger, kBg);
  sprite_.drawString(g.espNowReady ? chLabel : "down", 12, 86, 2);

  sprite_.setTextColor(kMuted, kBg);
  sprite_.drawString("SPP", 120, 72, 1);
  sprite_.setTextColor(g.sppConnected ? kAccent : kInk, kBg);
  sprite_.drawString(g.sppTarget[0] ? g.sppTarget : "-", 120, 86, 1);

  if (g.lastError[0]) {
    sprite_.setTextColor(kDanger, kBg);
    sprite_.drawString(g.lastError, 12, 112, 1);
  }
  sprite_.pushSprite(0, 0);
#else
  (void)store;
#endif
}

void TtgoStatusDisplay::drawEsp(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  drawChrome("ESP");
  const auto e = store.espHealth();
  char c0[24];
  char c1[24];
  char heap[24];
  char temp[24];
  std::snprintf(c0, sizeof(c0), "C0 %u%%", e.cpuCore0Percent);
  std::snprintf(c1, sizeof(c1), "C1 %u%%", e.cpuCore1Percent);
  std::snprintf(heap, sizeof(heap), "%lu B", static_cast<unsigned long>(e.freeHeapBytes));
  std::snprintf(temp, sizeof(temp), "%.1f C", static_cast<double>(e.chipTemperatureC));
  drawMetric(12, 36, "Radio core", c0);
  drawMetric(120, 36, "App core", c1);
  drawMetric(12, 78, "Heap", heap);
  drawMetric(120, 78, "Chip", temp);
  sprite_.pushSprite(0, 0);
#else
  (void)store;
#endif
}

void TtgoStatusDisplay::render(const telemetry::ITelemetryStore& store) {
  if (!ready_) {
    return;
  }
  if (pageIndex_ == 0) {
    drawOverview(store);
  } else if (pageIndex_ == 1) {
    drawGateway(store);
  } else {
    drawEsp(store);
  }
}

}  // namespace xt369p::display
