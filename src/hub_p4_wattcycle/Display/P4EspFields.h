#pragma once

#include "Display/P4MacText.h"

namespace wattcycle::display {

/// This panel's health. `deviceMac` is the same STA address shown on Gateway as hub.
struct P4EspFields {
  int heapKb = 0;
  int cpu0 = 0;
  int cpu1 = 0;
  int tempC = 0;
  int uptimeSec = 0;
  /// Free internal heap on the C6. Negative until the coprocessor answers.
  int c6HeapKb = -1;
  char deviceMac[kP4MacText] = {};
  /// STA address of the C6 once Wi-Fi has a lease. Empty until then.
  char staIp[16] = {};
  bool sdReady = false;
  int sdUsedMb = 0;
  int sdTotalMb = 0;
};

}  // namespace wattcycle::display
