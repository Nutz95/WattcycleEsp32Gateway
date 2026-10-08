#pragma once

#include <cstdint>

namespace xt369p::meter {

/// Latest DC report decoded from ATorch XT369P SPP frames.
struct WattmeterTelemetry {
  bool valid = false;
  float voltageV = 0.0f;
  float currentA = 0.0f;
  float powerW = 0.0f;
  float capacityAh = 0.0f;
  float energyWh = 0.0f;
  float temperatureC = 0.0f;
  float pricePerKwh = 0.0f;
  uint32_t runtimeS = 0;
  uint8_t backlightS = 0;
  bool checksumOk = false;
  uint32_t updatedAtMs = 0;
  uint32_t frameCount = 0;
};

}  // namespace xt369p::meter
