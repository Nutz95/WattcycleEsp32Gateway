#pragma once

#ifndef WATTCYCLE_DISPLAY_HUB_PAGE_H
#define WATTCYCLE_DISPLAY_HUB_PAGE_H

#include <cstdint>

namespace wattcycle::display {

/// Monitoring pages on the P4. Menu strip order lives in HubMenuOrder.h.
enum class HubPage : uint8_t {
  Overview = 0,
  Wattcycle,
  Solar,
  EcoFlow,
  Cells,
  Temps,
  Alerts,
  Gateway,
  Esp,
  Account,
  Camera,
  Plugs,
  Smoke,
  Link
};

constexpr uint8_t kHubPageCount = static_cast<uint8_t>(HubPage::Link) + 1u;

}  // namespace wattcycle::display

#endif  // WATTCYCLE_DISPLAY_HUB_PAGE_H
