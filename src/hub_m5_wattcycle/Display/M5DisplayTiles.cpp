#include "Display/M5DisplayTiles.h"

#ifndef UNIT_TEST

namespace wattcycle::display {

uint16_t statusTileFill(bool ok) {
  return ok ? kColorOk : kColorBad;
}

uint16_t warnTileFill(bool ok) {
  return ok ? kColorOk : kColorWarn;
}

uint16_t signedTileFill(float value, uint16_t idleFill) {
  if (value > 0.05f) {
    return kColorOk;
  }
  if (value < -0.05f) {
    return kColorBad;
  }
  return idleFill;
}

void drawTile(M5Canvas& canvas, int x, int y, int w, int h, uint16_t fill,
              const char* title, const char* value, uint8_t valueSize) {
  canvas.fillRoundRect(x, y, w, h, kTileRadius, fill);
  canvas.setTextColor(TFT_WHITE, fill);
  canvas.setTextSize(1);
  canvas.setCursor(x + 8, y + 8);
  canvas.print(title != nullptr ? title : "");
  canvas.setTextSize(valueSize);
  canvas.setCursor(x + 8, y + (valueSize >= 2 ? 28 : 30));
  canvas.print(value != nullptr ? value : "--");
}

void drawSplashFrame(M5Canvas& canvas) {
  canvas.fillSprite(kColorBg);
  canvas.fillRoundRect(16, 40, 288, 100, 4, kColorTileA);
  canvas.fillRoundRect(16, 156, 288, 40, 4, kColorTileB);
  canvas.setTextColor(TFT_WHITE, kColorTileA);
  canvas.setTextSize(2);
  canvas.setCursor(36, 68);
  canvas.print("WattCycle");
  canvas.setTextSize(1);
  canvas.setCursor(36, 100);
  canvas.print("M5 hub");
  canvas.setTextColor(TFT_WHITE, kColorTileB);
  canvas.setCursor(36, 170);
  canvas.print("Starting...");
}

void drawFooterTile(M5Canvas& canvas, int x, int y, int w, int h, const char* top,
                    const char* bottom) {
  canvas.fillRoundRect(x, y, w, h, kTileRadius, kColorFooterTile);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, kColorFooterTile);
  canvas.setCursor(x + 8, y + 6);
  canvas.print(top != nullptr ? top : "");
  canvas.setCursor(x + 8, y + 20);
  canvas.print(bottom != nullptr ? bottom : "");
}

}  // namespace wattcycle::display

#endif
