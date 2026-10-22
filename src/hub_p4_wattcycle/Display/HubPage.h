#pragma once

#include <cstdint>

namespace wattcycle::display {

/// Monitoring pages on the P4, in the same order as the web tabs.
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
  Link
};

/// Bottom menu stops at Esp. Link opens from the status icons.
constexpr uint8_t kHubMenuCount = static_cast<uint8_t>(HubPage::Esp) + 1u;
constexpr uint8_t kHubPageCount = static_cast<uint8_t>(HubPage::Link) + 1u;

}  // namespace wattcycle::display
