#pragma once

#include <cstdint>

namespace boards::dongle_s3 {

/// Portable ESP health lines for the shared dongle ESP page (no product types).
struct DongleEspHealthLines {
  const char* macAddress = "";
  uint8_t cpuCore0Percent = 0;
  uint8_t cpuCore1Percent = 0;
  uint32_t freeHeapBytes = 0;
  uint32_t uptimeSeconds = 0;
  float chipTemperatureC = 0.0f;
};

}  // namespace boards::dongle_s3
