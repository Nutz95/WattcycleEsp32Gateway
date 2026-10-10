#pragma once

#include "EcoFlow/Models/PowerStationTelemetry.h"

#include <cstdint>

namespace ecoflow::ble {

/// Cross-task publish view: copy under EcoFlowBleClient mutex, then use freely.
struct EcoFlowBleSnapshot {
  models::PowerStationTelemetry telemetry{};
  bool connected = false;
  bool authenticated = false;
  uint32_t notifyCount = 0;
  uint32_t telemetryCount = 0;
  char lastError[96] = {};
};

}  // namespace ecoflow::ble
