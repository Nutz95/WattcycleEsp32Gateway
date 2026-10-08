#pragma once

#include <cstdint>

namespace wattcycle::bms {

/// Scaling factors from the Wattcycle analog-quantity payload (DP 140).
struct ProtocolScaling {
  static constexpr float kCellVoltageDivisor = 1000.0f;
  static constexpr float kModuleVoltageDivisor = 100.0f;
  static constexpr float kCapacityDivisor = 10.0f;
  static constexpr float kTemperatureDivisor = 10.0f;
  static constexpr uint16_t kTemperatureKelvinOffset = 2730;
  static constexpr float kCurrentDecimalDivisor = 10.0f;
  static constexpr uint8_t kCurrentSignMask = 0x80;
  static constexpr uint8_t kCurrentDecimalMask = 0x40;
  static constexpr uint8_t kCurrentMagnitudeMask = 0x3F;
  static constexpr uint8_t kMinTemperatureCount = 2;
  static constexpr size_t kFixedTailFieldBytes = 12;
  static constexpr size_t kStateOfHealthFieldBytes = 2;
};

}  // namespace wattcycle::bms
