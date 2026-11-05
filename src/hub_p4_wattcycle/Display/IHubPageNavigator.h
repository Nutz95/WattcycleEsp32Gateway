#pragma once

#include <cstdint>

namespace wattcycle::display {

/// Opens a monitoring page from overview widgets (menu, cards).
class IHubPageNavigator {
 public:
  virtual ~IHubPageNavigator() = default;
  /// Switch the active HubPage by ordinal.
  virtual void showPage(uint8_t page) = 0;
};

}  // namespace wattcycle::display
