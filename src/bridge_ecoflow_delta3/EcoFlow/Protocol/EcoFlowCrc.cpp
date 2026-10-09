#include "EcoFlow/Protocol/EcoFlowCrc.h"

namespace ecoflow::protocol {

uint8_t EcoFlowCrc::crc8Smbus(const uint8_t* data, size_t length) {
  uint8_t crc = 0;
  if (data == nullptr) {
    return crc;
  }
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      if ((crc & 0x80U) != 0) {
        crc = static_cast<uint8_t>((crc << 1) ^ 0x07U);
      } else {
        crc = static_cast<uint8_t>(crc << 1);
      }
    }
  }
  return crc;
}

uint16_t EcoFlowCrc::crc16Arc(const uint8_t* data, size_t length) {
  uint16_t crc = 0;
  if (data == nullptr) {
    return crc;
  }
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      if ((crc & 0x0001U) != 0) {
        crc = static_cast<uint16_t>((crc >> 1) ^ 0xA001U);
      } else {
        crc = static_cast<uint16_t>(crc >> 1);
      }
    }
  }
  return crc;
}

}  // namespace ecoflow::protocol
