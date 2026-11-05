#pragma once

#ifndef WATTCYCLE_DISPLAY_HUB_MENU_ORDER_H
#define WATTCYCLE_DISPLAY_HUB_MENU_ORDER_H

#include "Display/HubPage.h"

namespace wattcycle::display {

/// Bottom-strip order (independent of HubPage enum ordinals). Link is omitted.
constexpr HubPage kHubMenuOrder[] = {
    HubPage::Overview, HubPage::Camera, HubPage::Wattcycle, HubPage::Solar, HubPage::EcoFlow,
    HubPage::Cells,    HubPage::Temps,  HubPage::Alerts,    HubPage::Gateway, HubPage::Esp,
    HubPage::Plugs,    HubPage::Smoke,  HubPage::Account};

constexpr uint8_t kHubMenuCount =
    static_cast<uint8_t>(sizeof(kHubMenuOrder) / sizeof(kHubMenuOrder[0]));

/// Index of `page` in `kHubMenuOrder`, or -1 when not on the strip.
inline int hubMenuIndexOf(HubPage page) {
  for (uint8_t i = 0; i < kHubMenuCount; ++i) {
    if (kHubMenuOrder[i] == page) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace wattcycle::display

#endif  // WATTCYCLE_DISPLAY_HUB_MENU_ORDER_H
