#pragma once

#include <cstddef>
#include <cstdint>

namespace ecoflow::protocol {

/// CRC helpers matching EcoFlow BLE framing (CRC8-SMBus + CRC16-ARC).
class EcoFlowCrc {
 public:
  /// CRC8-SMBus over payload (EcoFlow header checksum).
  static uint8_t crc8Smbus(const uint8_t* data, size_t length);
  /// CRC16-ARC over payload (EcoFlow frame tail).
  static uint16_t crc16Arc(const uint8_t* data, size_t length);
};

}  // namespace ecoflow::protocol
