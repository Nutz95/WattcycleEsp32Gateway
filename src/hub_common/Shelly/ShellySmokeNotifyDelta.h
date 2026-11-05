#pragma once

#include "Shelly/ShellySmokeLimits.h"

namespace wattcycle::shelly {

/// Fields extracted from one Plus Smoke NotifyStatus / NotifyFullStatus frame.
struct ShellySmokeNotifyDelta {
  char id[kShellyIdCapacity] = {};
  bool alarm = false;
  bool mute = false;
  float batteryPercent = 0;
  float batteryVolts = 0;
  bool hasAlarm = false;
  bool hasMute = false;
  bool hasBatteryPercent = false;
  bool hasBatteryVolts = false;
  bool full = false;
  bool ok = false;
};

}  // namespace wattcycle::shelly
