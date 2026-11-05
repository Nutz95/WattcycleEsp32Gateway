#pragma once

#include "Shelly/ShellySmokeLimits.h"

#include <cstdint>

namespace wattcycle::shelly {

/// Merged live reading for one smoke detector (authorized or posting-only).
struct ShellySmokeReading {
  char id[kShellyIdCapacity] = {};
  char name[kShellyNameCapacity] = {};
  bool alarm = false;
  bool mute = false;
  float batteryPercent = 0;
  float batteryVolts = 0;
  bool online = false;
  bool authorized = false;
  bool hasAlarm = false;
  bool hasMute = false;
  bool hasBatteryPercent = false;
  bool hasBatteryVolts = false;
  uint32_t lastMs = 0;
};

}  // namespace wattcycle::shelly
