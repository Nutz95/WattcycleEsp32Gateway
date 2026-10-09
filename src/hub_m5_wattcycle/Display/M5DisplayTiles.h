#pragma once

#ifndef UNIT_TEST
#include <M5Unified.h>
#endif

namespace wattcycle::display {

#ifndef UNIT_TEST
// Metro tiles: white text always; tile fill encodes status / sign (RGB332).
constexpr uint16_t kColorBg = 0x08C4;
constexpr uint16_t kColorTileA = 0x03BF;
constexpr uint16_t kColorTileB = 0x0451;
constexpr uint16_t kColorTileC = 0xEAA0;
constexpr uint16_t kColorTileD = 0x901A;
constexpr uint16_t kColorFooterTile = 0x0257;
constexpr uint16_t kColorOk = 0x05E0;
constexpr uint16_t kColorBad = 0xF800;
constexpr uint16_t kColorWarn = 0xEAA0;
constexpr int kTileRadius = 2;

uint16_t statusTileFill(bool ok);
uint16_t warnTileFill(bool ok);
uint16_t signedTileFill(float value, uint16_t idleFill);

void drawTile(M5Canvas& canvas, int x, int y, int w, int h, uint16_t fill,
              const char* title, const char* value, uint8_t valueSize = 2);
void drawSplashFrame(M5Canvas& canvas);
void drawFooterTile(M5Canvas& canvas, int x, int y, int w, int h, const char* top,
                    const char* bottom);
#endif

}  // namespace wattcycle::display
