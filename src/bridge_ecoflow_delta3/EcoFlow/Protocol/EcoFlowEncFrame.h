#pragma once

#include <cstdint>

namespace ecoflow::protocol {

/// EncPacket wrapper fields (`0x5A5A…`) before/after session crypto.
struct EcoFlowEncFrame {
  uint8_t frameType = 0;
  uint8_t payloadType = 0;
  const uint8_t* payload = nullptr;
  uint16_t payloadLength = 0;
};

}  // namespace ecoflow::protocol
