#pragma once

#include "boards/dongle_s3/Rgb565Color.h"

#include <cstdint>

namespace boards::dongle_s3 {

/// Shared RGB565 palette for Pocket/LilyGO dongle status UIs.
struct DonglePanelPalette {
  static constexpr uint16_t kBackground = Rgb565Color::fromRgb(0, 0, 0);
  static constexpr uint16_t kBannerBackground = Rgb565Color::fromRgb(28, 28, 28);
  static constexpr uint16_t kBannerText = Rgb565Color::fromRgb(255, 255, 255);
  static constexpr uint16_t kAccent = Rgb565Color::fromRgb(0, 220, 120);
  static constexpr uint16_t kMuted = Rgb565Color::fromRgb(148, 168, 188);
  static constexpr uint16_t kDanger = Rgb565Color::fromRgb(255, 48, 48);
  static constexpr uint16_t kWarn = Rgb565Color::fromRgb(255, 220, 0);
  static constexpr uint16_t kText = Rgb565Color::fromRgb(255, 255, 255);
};

}  // namespace boards::dongle_s3
