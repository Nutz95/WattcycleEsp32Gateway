#pragma once

#include <cstdint>

namespace ecoflow::protocol {

/// Plaintext EcoFlow Packet fields (`0xAA…` framing).
struct EcoFlowPacket {
  uint8_t src = 0;
  uint8_t dst = 0;
  uint8_t cmdSet = 0;
  uint8_t cmdId = 0;
  uint8_t dsrc = 1;
  uint8_t ddst = 1;
  uint8_t version = 3;
  uint8_t productId = 0;
  uint8_t seq[4] = {0, 0, 0, 0};
  const uint8_t* payload = nullptr;
  uint16_t payloadLength = 0;
};

}  // namespace ecoflow::protocol
