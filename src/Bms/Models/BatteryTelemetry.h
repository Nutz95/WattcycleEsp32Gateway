#pragma once

#include <cstdint>

#include "Bms/Protocol/ProtocolConstants.h"

namespace wattcycle::bms {

/// Snapshot of BMS analog quantity (DP 140).
struct BatteryTelemetry {
  bool valid = false;
  uint8_t cellCount = 0;
  float cellVoltages[kMaxCellCount] = {};
  uint8_t temperatureCount = 0;
  float mosTemperatureC = 0.0f;
  float pcbTemperatureC = 0.0f;
  float cellTemperaturesC[kMaxCellCount] = {};
  float currentAmps = 0.0f;
  float moduleVoltage = 0.0f;
  float remainingCapacityAh = 0.0f;
  float totalCapacityAh = 0.0f;
  uint16_t cycleNumber = 0;
  float designCapacityAh = 0.0f;
  uint16_t stateOfChargePercent = 0;
  uint16_t stateOfHealthPercent = 0;
  float powerWatts = 0.0f;
  uint32_t updatedAtMs = 0;
};

struct ProductInfo {
  bool valid = false;
  char firmwareVersion[21] = {};
  char manufacturerName[21] = {};
  char serialNumber[21] = {};
};

struct WarningFlags {
  bool valid = false;
  uint8_t statusRegister1 = 0;
  uint8_t statusRegister2 = 0;
  uint8_t statusRegister5 = 0;
  uint8_t warningRegister1 = 0;
  uint8_t warningRegister2 = 0;
  bool hasActiveProtection = false;
  bool hasFault = false;
};

}  // namespace wattcycle::bms
