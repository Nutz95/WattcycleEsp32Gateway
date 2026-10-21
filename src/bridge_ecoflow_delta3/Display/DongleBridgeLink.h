#pragma once

#include <cstdint>

namespace ecoflow::display {

/// ESP-NOW TX state for the dongle link page.
struct DongleBridgeLink {
  bool espNowReady = false;
  uint8_t channel = 0;
  const char* peerMac = "";
};

}  // namespace ecoflow::display
