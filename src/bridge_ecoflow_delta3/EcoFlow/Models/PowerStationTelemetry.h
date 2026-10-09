#pragma once

#include <cstdint>

namespace ecoflow::models {

/// Latest read-only snapshot from DisplayPropertyUpload (partial merges OK).
struct PowerStationTelemetry {
  bool valid = false;
  uint8_t socPercent = 0;
  int16_t acOutputW = 0;
  int16_t acInputW = 0;
  int16_t dcOutputW = 0;
  int16_t solarInputW = 0;
  int16_t usbOutputW = 0;
  int16_t tempC = 0;
  bool haveTemperature = false;
  uint16_t remainMinutes = 0;
  bool acOutputOn = false;
  bool dcOutputOn = false;
  bool usbOutputOn = false;
  uint32_t updatedAtMs = 0;
};

}  // namespace ecoflow::models
