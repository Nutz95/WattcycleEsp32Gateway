#pragma once

#include "Shelly/ShellyPlugLimits.h"

namespace wattcycle::shelly {

/// Fields present in one NotifyStatus / NotifyFullStatus datagram.
struct ShellyNotifyDelta {
  bool ok = false;
  bool full = false;
  char id[kShellyIdCapacity] = {};
  bool hasApower = false;
  float apower = 0;
  bool hasVoltage = false;
  float voltage = 0;
  bool hasCurrent = false;
  float current = 0;
  bool hasAenergy = false;
  float aenergyWh = 0;
  bool hasOutput = false;
  bool output = false;
};

}  // namespace wattcycle::shelly
