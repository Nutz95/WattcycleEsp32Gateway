#pragma once

#include "boards/ttgo_tdisplay/BoardPins.h"

namespace wattcycle::display {

constexpr int kButtonCancelGpio = boards::ttgo_tdisplay::kButtonCancelGpio;
constexpr int kButtonConfirmGpio = boards::ttgo_tdisplay::kButtonConfirmGpio;
constexpr int kButtonPrevGpio = kButtonCancelGpio;
constexpr int kButtonNextGpio = kButtonConfirmGpio;
constexpr int kBacklightGpio = boards::ttgo_tdisplay::kBacklightGpio;

constexpr int kDisplayWidth = boards::ttgo_tdisplay::kDisplayWidth;
constexpr int kDisplayHeight = boards::ttgo_tdisplay::kDisplayHeight;

constexpr uint8_t kPageCount = 6;

}  // namespace wattcycle::display
