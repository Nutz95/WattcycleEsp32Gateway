#pragma once

#include "Shelly/IShellyPlugHub.h"
#include "Shelly/IShellySmokeHub.h"

#include <WiFiUdp.h>
#include <ctime>

namespace wattcycle::panel {

/// Listens on UDP 2267 and demuxes NotifyStatus frames to plug and smoke hubs.
class P4ShellyUdp {
 public:
  P4ShellyUdp(shelly::IShellyPlugHub& plugs, shelly::IShellySmokeHub& smoke);

  bool begin();
  void loop(uint32_t nowMs, std::time_t epochUtc);

 private:
  shelly::IShellyPlugHub& plugs_;
  shelly::IShellySmokeHub& smoke_;
  WiFiUDP udp_;
  bool started_ = false;
  char buffer_[shelly::kShellyUdpBuffer + 1] = {};
};

}  // namespace wattcycle::panel
