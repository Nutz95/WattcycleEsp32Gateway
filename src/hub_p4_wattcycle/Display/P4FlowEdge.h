#pragma once

namespace wattcycle::display {

/// One arrow on the overview. `reverse` points back toward the EcoFlow.
struct P4FlowEdge {
  bool active = false;
  bool reverse = false;
};

}  // namespace wattcycle::display
