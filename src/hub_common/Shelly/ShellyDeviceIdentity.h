#pragma once

#include "Shelly/ShellyPlugLimits.h"

namespace wattcycle::shelly {

/// Whitelist entry shared by Shelly plugs and smoke detectors.
struct ShellyDeviceIdentity {
  char id[kShellyIdCapacity] = {};
  char name[kShellyNameCapacity] = {};
};

}  // namespace wattcycle::shelly
