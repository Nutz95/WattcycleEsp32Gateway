#pragma once

#include "Shelly/ShellyDeviceIdentity.h"
#include "Shelly/ShellySmokeReading.h"

namespace wattcycle::shelly {

/// One RAM slot: identity and merged smoke reading.
struct ShellySmokeSlot {
  ShellyDeviceIdentity identity;
  ShellySmokeReading reading;
  bool authorized = false;
  bool used = false;
};

}  // namespace wattcycle::shelly
