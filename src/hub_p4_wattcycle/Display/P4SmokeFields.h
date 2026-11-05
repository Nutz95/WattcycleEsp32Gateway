#pragma once

#include "Shelly/ShellySmokeLimits.h"

#include <cstdint>

namespace wattcycle::display {

/// Panel document for authorized Shelly Plus Smoke detectors.
struct P4SmokeFields {
  bool anyAlarm = false;
  uint8_t count = 0;
  char names[shelly::kMaxShellySmoke][shelly::kShellyNameCapacity] = {};
  bool alarm[shelly::kMaxShellySmoke] = {};
  bool online[shelly::kMaxShellySmoke] = {};
  float batteryPercent[shelly::kMaxShellySmoke] = {};
};

}  // namespace wattcycle::display
