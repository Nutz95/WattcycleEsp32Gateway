#pragma once

namespace wattcycle::display {

/// Row layout for Shelly detail pages (must match mountRows captions).
constexpr int kPlugsSummaryRow = 0;
constexpr int kPlugsDeviceRow0 = 1;
constexpr int kPlugsDeviceSlots = 8;
constexpr int kPlugsRowCount = kPlugsDeviceRow0 + kPlugsDeviceSlots;

constexpr int kSmokeStatusRow = 0;
constexpr int kSmokeDeviceRow0 = 1;
constexpr int kSmokeDeviceSlots = 4;
constexpr int kSmokeRowCount = kSmokeDeviceRow0 + kSmokeDeviceSlots;

}  // namespace wattcycle::display
