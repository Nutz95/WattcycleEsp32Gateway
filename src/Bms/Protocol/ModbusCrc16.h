#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::bms {

/// Modbus CRC16 as used by the Wattcycle Android app (init 0xFF/0xFF).
class ModbusCrc16 {
 public:
  static uint16_t compute(const uint8_t* data, size_t length);
  static bool verifyFrame(const uint8_t* frame, size_t length);
};

}  // namespace wattcycle::bms
