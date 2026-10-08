#pragma once

#include <cstddef>
#include <cstdint>

#include "Bms/Models/BatteryTelemetry.h"

namespace wattcycle::bms {

/// Parses Analog Quantity (DP 140) payload into BatteryTelemetry.
class AnalogQuantityParser {
 public:
  static bool parse(const uint8_t* data, size_t length, BatteryTelemetry& out);
  static float parseSignedCurrent(uint8_t highByte, uint8_t lowByte);
};

}  // namespace wattcycle::bms
