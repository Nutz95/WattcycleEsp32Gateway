#pragma once

#include "Shelly/ShellyPlugLimits.h"

namespace wattcycle::shelly {

/// Whitelist entry: stable Shelly device id plus display name on the hub.
struct ShellyPlugIdentity {
  char id[kShellyIdCapacity] = {};
  char name[kShellyNameCapacity] = {};
};

}  // namespace wattcycle::shelly
