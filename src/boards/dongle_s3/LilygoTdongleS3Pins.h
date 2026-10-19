#pragma once

#include <cstdint>

namespace boards::dongle_s3 {

/// LILYGO T-Dongle-S3 — TFT_eSPI Setup209 (ST7735 80x160).
constexpr int kTftMosi = 3;
constexpr int kTftSclk = 5;
constexpr int kTftCs = 4;
constexpr int kTftDc = 2;
constexpr int kTftRst = 1;
constexpr int kBacklightGpio = 38;  // TFT_LEDA, active LOW
constexpr bool kBacklightActiveHigh = false;
constexpr int kButtonGpio = 0;
constexpr int kLedDataGpio = 40;   // APA102 DI
constexpr int kLedClockGpio = 39;  // APA102 CI
constexpr int kDisplayWidth = 160;
constexpr int kDisplayHeight = 80;
constexpr int kTftPortraitWidth = 80;
constexpr int kTftPortraitHeight = 160;
constexpr uint32_t kLongPressMs = 800;
constexpr const char* kBoardName = "LilyGO-T-Dongle-S3";

}  // namespace boards::dongle_s3
