#include "boards/dongle_s3/DonglePanelShell.h"

#include "boards/dongle_s3/ActiveBoardPins.h"
#include "boards/dongle_s3/DonglePanelPalette.h"

#include <Arduino.h>

#include <cstdio>

namespace boards::dongle_s3 {

bool DonglePanelShell::begin(const char* logTag, const char* splashText) {
  Serial.printf("[%s] TFT init heap=%u psram=%u BL=%s\n", logTag, ESP.getFreeHeap(),
                ESP.getFreePsram(), kBacklightGpio >= 0 ? "GPIO" : "hard-wired");
  tft_.begin();
  tft_.setRotation(1);
  if (kBacklightGpio >= 0) {
    pinMode(kBacklightGpio, OUTPUT);
    digitalWrite(kBacklightGpio, LOW);
  }
  backlightOn_ = true;

  if (!frame_.begin(kDisplayWidth, kDisplayHeight)) {
    Serial.printf("[%s] double-buffer failed\n", logTag);
    return false;
  }

  TFT_eSprite& canvas = frame_.drawTarget();
  canvas.fillSprite(DonglePanelPalette::kBackground);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(DonglePanelPalette::kText, DonglePanelPalette::kBackground);
  canvas.drawString(splashText != nullptr ? splashText : "", 4, 28, 2);
  frame_.present();
  ready_ = true;
  Serial.printf("[%s] ready\n", logTag);
  return true;
}

void DonglePanelShell::setBacklight(bool on) {
  backlightOn_ = on;
  if (kBacklightGpio < 0) {
    if (!on && ready_) {
      frame_.drawTarget().fillSprite(DonglePanelPalette::kBackground);
      frame_.present();
      tft_.writecommand(kSt7735DispOff);
    } else if (on) {
      tft_.writecommand(kSt7735DispOn);
    }
    return;
  }
  const bool level = kBacklightActiveHigh ? on : !on;
  digitalWrite(kBacklightGpio, level ? HIGH : LOW);
}

bool DonglePanelShell::isBacklightOn() const {
  return backlightOn_;
}

bool DonglePanelShell::ready() const {
  return ready_;
}

TFT_eSprite& DonglePanelShell::canvas() {
  return frame_.drawTarget();
}

void DonglePanelShell::drawChrome(const char* title, uint8_t pageIndex, uint8_t pageCount) {
  TFT_eSprite& canvas = frame_.drawTarget();
  canvas.fillSprite(DonglePanelPalette::kBackground);
  canvas.fillRoundRect(2, 2, kDisplayWidth - 4, 14, 3, DonglePanelPalette::kBannerBackground);
  canvas.setTextColor(DonglePanelPalette::kBannerText, DonglePanelPalette::kBannerBackground);
  canvas.setTextSize(1);
  canvas.drawString(title != nullptr ? title : "", 6, 5);
  const int dotsX = kDisplayWidth - 6 - static_cast<int>(pageCount) * 8;
  for (uint8_t i = 0; i < pageCount; ++i) {
    const int x = dotsX + static_cast<int>(i) * 8;
    canvas.fillCircle(x, 8, 2,
                      i == pageIndex ? DonglePanelPalette::kAccent : DonglePanelPalette::kMuted);
  }
}

void DonglePanelShell::drawEspPage(const char* title, uint8_t pageIndex, uint8_t pageCount,
                                   const DongleEspHealthLines& lines) {
  drawChrome(title, pageIndex, pageCount);
  TFT_eSprite& canvas = frame_.drawTarget();
  char line[36];
  canvas.setTextColor(DonglePanelPalette::kText, DonglePanelPalette::kBackground);
  canvas.drawString(lines.macAddress != nullptr && lines.macAddress[0] != '\0' ? lines.macAddress
                                                                              : "(no MAC)",
                    6, 18);
  std::snprintf(line, sizeof(line), "CPU0 %u%%  CPU1 %u%%", lines.cpuCore0Percent,
                lines.cpuCore1Percent);
  canvas.setTextColor(DonglePanelPalette::kText, DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 32);
  std::snprintf(line, sizeof(line), "heap %uKB  up %lus", lines.freeHeapBytes / 1024u,
                static_cast<unsigned long>(lines.uptimeSeconds));
  canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 46);
  std::snprintf(line, sizeof(line), "chip %.0fC", static_cast<double>(lines.chipTemperatureC));
  canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 60);
}

void DonglePanelShell::present() {
  frame_.present();
}

}  // namespace boards::dongle_s3
