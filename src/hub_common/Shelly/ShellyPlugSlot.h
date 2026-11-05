#pragma once

#include "Shelly/ShellyDeviceIdentity.h"
#include "Shelly/ShellyPlugReading.h"

namespace wattcycle::shelly {

/// One RAM slot: identity, merged reading, and day Wh baseline.
struct ShellyPlugSlot {
  ShellyDeviceIdentity identity;
  ShellyPlugReading reading;
  float dayBaselineWh = 0;
  bool hasBaseline = false;
  bool authorized = false;
  bool used = false;
};

}  // namespace wattcycle::shelly
