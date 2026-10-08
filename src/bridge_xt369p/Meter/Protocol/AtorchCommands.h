#pragma once

#include <cstddef>
#include <cstdint>

namespace xt369p::meter {

enum class MeterCommand : uint8_t {
  None = 0,
  ResetWh = 0x01,
  ResetAh = 0x02,
  ResetDuration = 0x03,
  ResetAll = 0x05,
};

/// Encodes FF55 type-0x11 command frames for DC meters (device type 0x02).
class AtorchCommands {
 public:
  static constexpr size_t kFrameLen = 10;
  static constexpr uint8_t kDeviceTypeDc = 0x02;

  static bool encode(MeterCommand command, uint8_t out[kFrameLen]);
  static const char* toWireName(MeterCommand command);
  static MeterCommand fromWireName(const char* name);
};

}  // namespace xt369p::meter
