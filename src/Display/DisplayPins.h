#pragma once

namespace wattcycle::display {

/// TTGO T-Display front buttons (active low). No capacitive touch on this board.
constexpr int kButtonPrevGpio = 0;
constexpr int kButtonNextGpio = 35;
constexpr int kBacklightGpio = 4;

constexpr int kDisplayWidth = 240;
constexpr int kDisplayHeight = 135;

constexpr uint8_t kPageCount = 6;

}  // namespace wattcycle::display
