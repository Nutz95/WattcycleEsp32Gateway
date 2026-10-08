#pragma once

#include <cstdint>

namespace wattcycle::telemetry {

/// Latest solar wattmeter snapshot bridged from the XT369P SPP ESP via ESP-NOW.
struct SolarBridgeTelemetry {
  bool linkFresh = false;
  bool sppConnected = false;
  bool meterValid = false;
  bool checksumOk = false;
  float voltageV = 0.0f;
  float currentA = 0.0f;
  float powerW = 0.0f;
  float capacityAh = 0.0f;
  float energyWh = 0.0f;
  float temperatureC = 0.0f;
  uint32_t runtimeS = 0;
  uint32_t frameCount = 0;
  uint32_t seq = 0;
  uint32_t receivedAtMs = 0;
  char sppTarget[32] = {};
  char lastError[64] = {};
};

}  // namespace wattcycle::telemetry
