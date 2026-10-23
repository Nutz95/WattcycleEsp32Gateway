#pragma once

#include "Shelly/ShellyPlugIdentity.h"
#include "Shelly/ShellyPlugReading.h"

namespace wattcycle::shelly {

/// One RAM slot: identity, merged reading, and day Wh baseline.
struct ShellyPlugSlot {
  ShellyPlugIdentity identity;
  ShellyPlugReading reading;
  float dayBaselineWh = 0;
  bool hasBaseline = false;
  bool authorized = false;
  bool used = false;
};

}  // namespace wattcycle::shelly
