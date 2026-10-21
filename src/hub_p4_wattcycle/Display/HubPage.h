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
  Esp
};

constexpr uint8_t kHubPageCount = static_cast<uint8_t>(HubPage::Esp) + 1u;

}  // namespace wattcycle::display
