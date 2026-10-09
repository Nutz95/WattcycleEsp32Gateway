#pragma once

#include <cstdint>

#include "Telemetry/EspHealth.h"

namespace wattcycle::telemetry {

/// Latest EcoFlow power-station snapshot bridged from the S3 BLE ESP via ESP-NOW.
struct EcoFlowBridgeTelemetry {
  bool linkFresh = false;
  bool bleConnected = false;
  bool meterValid = false;
  bool acOutputOn = false;
  bool dcOutputOn = false;
  bool usbOutputOn = false;
  bool haveTemperature = false;
  bool espNowEncrypted = false;
  bool bridgeEspValid = false;
  uint8_t socPercent = 0;
  int16_t acOutputW = 0;
  int16_t acInputW = 0;
  int16_t dcOutputW = 0;
  int16_t solarInputW = 0;
  int16_t usbOutputW = 0;
  float temperatureC = 0.0f;
  uint16_t remainMinutes = 0;
  uint32_t seq = 0;
  uint32_t telemetryCount = 0;
  uint32_t receivedAtMs = 0;
  char lastError[64] = {};
  EspHealth bridgeEsp{};
};

}  // namespace wattcycle::telemetry
