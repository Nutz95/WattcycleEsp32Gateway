#pragma once

namespace wattcycle::display {

/// Example or live controller health. Not a substitute for the chip's own counters.
struct P4EspFields {
  int heapKb = 0;
  int cpu0 = 0;
  int cpu1 = 0;
};

}  // namespace wattcycle::display
