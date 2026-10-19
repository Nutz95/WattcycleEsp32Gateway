#pragma once

#include <cstdint>

namespace boards::dongle_s3 {

/// Pocket-Dongle-S3-0.96 — vendor User_Setup + FPC schematic (LEDK hard-wired).
constexpr int kTftMosi = 11;
constexpr int kTftSclk = 10;
constexpr int kTftCs = 12;
constexpr int kTftDc = 13;
constexpr int kTftRst = 14;
constexpr int kBacklightGpio = -1;  // LEDK → GND via resistor
constexpr bool kBacklightActiveHigh = false;
constexpr int kButtonGpio = 0;
constexpr int kLedDataGpio = -1;
constexpr int kLedClockGpio = -1;
constexpr int kDisplayWidth = 160;
constexpr int kDisplayHeight = 80;
constexpr int kTftPortraitWidth = 80;
constexpr int kTftPortraitHeight = 160;
constexpr uint32_t kLongPressMs = 800;
constexpr const char* kBoardName = "Pocket-Dongle-S3-0.96";

}  // namespace boards::dongle_s3
