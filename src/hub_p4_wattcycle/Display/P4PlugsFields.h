#pragma once

#include "Shelly/ShellyPlugLimits.h"

#include <cstdint>

namespace wattcycle::display {

/// Panel document for authorized Shelly plugs.
struct P4PlugsFields {
  float totalWatts = 0;
  uint8_t count = 0;
  char names[shelly::kMaxShellyPlugs][shelly::kShellyNameCapacity] = {};
  float watts[shelly::kMaxShellyPlugs] = {};
  float whToday[shelly::kMaxShellyPlugs] = {};
  bool online[shelly::kMaxShellyPlugs] = {};
  bool output[shelly::kMaxShellyPlugs] = {};
};

}  // namespace wattcycle::display
