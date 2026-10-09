#pragma once

#include "EcoFlow/Protocol/EcoFlowEncFrame.h"
#include "EcoFlow/Protocol/EcoFlowPacket.h"

#include <cstddef>
#include <cstdint>

namespace ecoflow::protocol {

/// Encode/decode EcoFlow Packet and EncPacket wire frames.
class EcoFlowPacketCodec {
 public:
  static constexpr uint8_t kPacketPrefix = 0xAA;
  static constexpr uint8_t kEncPrefix0 = 0x5A;
  static constexpr uint8_t kEncPrefix1 = 0x5A;
  static constexpr size_t kMinPacketBytes = 20;
  static constexpr size_t kMinEncBytes = 8;

  /// Serialize plaintext Packet (`0xAA…`) into `out` (returns bytes written, 0 on failure).
  static size_t encodePacket(const EcoFlowPacket& packet, uint8_t* out, size_t outCapacity);

  /// Parse plaintext Packet from `data`. Payload points into `data` (no copy).
  static bool decodePacket(const uint8_t* data, size_t length, EcoFlowPacket& out);

  /// DELTA 3 / v19: XOR payload with seq[0] when non-zero; strip trailing `BB BB`.
  /// Mutates `payload` in place. Returns updated length.
  static uint16_t deobfuscatePayload(uint8_t version, uint8_t seq0, uint8_t* payload,
                                     uint16_t payloadLen);

  /// Serialize EncPacket wrapper (`0x5A5A…`) without encryption (caller encrypts payload).
  static size_t encodeEncFrame(const EcoFlowEncFrame& frame, uint8_t* out, size_t outCapacity);

  /// Parse one EncPacket; payload points into `data`; returns consumed bytes (0 = need more/fail).
  static size_t decodeEncFrame(const uint8_t* data, size_t length, EcoFlowEncFrame& out);
};

}  // namespace ecoflow::protocol
