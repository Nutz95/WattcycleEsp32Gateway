#pragma once

namespace wattcycle::display {

/// M5Stack Basic face buttons (active low via M5Unified).
constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
/// Overview / Pack / Solar / Gateway / ESP — must match M5StatusDisplay pages.
constexpr uint8_t kPageCount = 6;
constexpr int kFooterHeight = 40;
/// Content area above the A/B/C footer tiles.
constexpr int kContentHeight = kDisplayHeight - kFooterHeight;

}  // namespace wattcycle::display
