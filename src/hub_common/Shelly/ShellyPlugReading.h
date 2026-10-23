#pragma once

#include "Shelly/ShellyPlugLimits.h"

#include <cstdint>

namespace wattcycle::shelly {

/// Merged live reading for one plug (authorized or posting-only).
struct ShellyPlugReading {
  char id[kShellyIdCapacity] = {};
  char name[kShellyNameCapacity] = {};
  float watts = 0;
  float volts = 0;
  float amps = 0;
  float whTotal = 0;
  float whToday = 0;
  bool output = false;
  bool online = false;
  bool authorized = false;
  bool hasWatts = false;
  bool hasVolts = false;
  bool hasAmps = false;
  bool hasWhTotal = false;
  bool hasOutput = false;
  uint32_t lastMs = 0;
};

}  // namespace wattcycle::shelly
