#pragma once

namespace wattcycle::display {

/// TTGO T-Display front buttons (active low). No capacitive touch on this board.
/// Physical layout: GPIO0 = bottom (cancel / previous), GPIO35 = top (confirm / next).
constexpr int kButtonCancelGpio = 0;
constexpr int kButtonConfirmGpio = 35;
constexpr int kButtonPrevGpio = kButtonCancelGpio;
constexpr int kButtonNextGpio = kButtonConfirmGpio;
constexpr int kBacklightGpio = 4;

constexpr int kDisplayWidth = 240;
constexpr int kDisplayHeight = 135;

constexpr uint8_t kPageCount = 6;

}  // namespace wattcycle::display
