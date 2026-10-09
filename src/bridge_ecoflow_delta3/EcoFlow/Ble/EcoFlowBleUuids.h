#pragma once

namespace ecoflow::ble {

/// GATT UUIDs used by EcoFlow V3 BLE (DELTA 3 family).
struct EcoFlowBleUuids {
  static constexpr const char* kService = "00000001-0000-1000-8000-00805f9b34fb";
  static constexpr const char* kWrite = "00000002-0000-1000-8000-00805f9b34fb";
  static constexpr const char* kNotify = "00000003-0000-1000-8000-00805f9b34fb";
};

}  // namespace ecoflow::ble
