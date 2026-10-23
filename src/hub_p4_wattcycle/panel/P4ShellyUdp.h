#pragma once

#include "Shelly/IShellyPlugHub.h"

#include <WiFiUdp.h>
#include <ctime>

namespace wattcycle::panel {

/// Listens on UDP 2267 and feeds Shelly NotifyStatus frames into the hub.
class P4ShellyUdp {
 public:
  explicit P4ShellyUdp(shelly::IShellyPlugHub& hub);

  bool begin();
  void loop(uint32_t nowMs, std::time_t epochUtc);

 private:
  shelly::IShellyPlugHub& hub_;
  WiFiUDP udp_;
  bool started_ = false;
  char buffer_[shelly::kShellyUdpBuffer + 1] = {};
};

}  // namespace wattcycle::panel
