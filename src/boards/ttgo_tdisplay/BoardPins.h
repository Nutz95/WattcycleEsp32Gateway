#pragma once

#include <cstdint>

namespace boards::ttgo_tdisplay {

/// LILYGO TTGO T-Display — ST7789 240x135, dual front buttons.
constexpr int kButtonCancelGpio = 0;   // bottom / previous (active low)
constexpr int kButtonConfirmGpio = 35;  // top / next (active low, no pull-up)
constexpr int kBacklightGpio = 4;

constexpr int kDisplayWidth = 240;
constexpr int kDisplayHeight = 135;

}  // namespace boards::ttgo_tdisplay
