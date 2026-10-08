#pragma once

#include <cstddef>
#include <cstdint>

#include "Bms/Models/BatteryTelemetry.h"

namespace wattcycle::bms {

class WarningInfoParser {
 public:
  static bool parse(const uint8_t* data, size_t length, WarningFlags& out);
};

}  // namespace wattcycle::bms
