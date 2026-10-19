#pragma once

#include <cstdint>

namespace boards::dongle_s3 {

/// Pack 8-bit RGB into RGB565 for TFT_eSPI sprites / ST7735.
/// Use this instead of hand-picked 0xNNNN literals (easy to get BGR/endian wrong).
class Rgb565Color {
 public:
  /// Standard RGB565: RRRRR GGGGGG BBBBB (panel MADCTL BGR bit still applies in HW).
  static constexpr uint16_t fromRgb(uint8_t red, uint8_t green, uint8_t blue) {
    return static_cast<uint16_t>(((red & 0xF8u) << 8) | ((green & 0xFCu) << 3) | (blue >> 3));
  }
};

}  // namespace boards::dongle_s3
