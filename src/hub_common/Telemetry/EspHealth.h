#pragma once

#include <cstdint>

namespace wattcycle::telemetry {

struct EspHealth {
  uint8_t cpuCore0Percent = 0;
  uint8_t cpuCore1Percent = 0;
  float chipTemperatureC = 0.0f;
  uint32_t freeHeapBytes = 0;
  uint32_t uptimeSeconds = 0;
  /// Coprocessor free heap in KB. -1 on a hub that has no coprocessor.
  int coprocessorHeapKb = -1;
};

}  // namespace wattcycle::telemetry
