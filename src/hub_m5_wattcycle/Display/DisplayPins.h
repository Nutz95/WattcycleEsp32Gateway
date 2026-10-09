#pragma once

namespace wattcycle::display {

/// M5Stack Basic face buttons (active low via M5Unified).
constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
/// Overview / EcoFlow / Pack / Solar / Gateway / ESP / Temps — match M5StatusDisplay pages.
constexpr uint8_t kPageCount = 7;
constexpr int kFooterHeight = 40;
/// Content area above the A/B/C footer tiles.
constexpr int kContentHeight = kDisplayHeight - kFooterHeight;

#ifndef UNIT_TEST
extern const char* const kPageNames[kPageCount];
#endif

}  // namespace wattcycle::display
