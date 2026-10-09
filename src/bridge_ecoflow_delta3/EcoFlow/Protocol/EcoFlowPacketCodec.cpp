#include "EcoFlow/Protocol/EcoFlowPacketCodec.h"

#include "EcoFlow/Protocol/EcoFlowCrc.h"

#include <cstring>

namespace ecoflow::protocol {
namespace {

uint16_t readLe16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

void writeLe16(uint8_t* p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value & 0xFFU);
  p[1] = static_cast<uint8_t>((value >> 8) & 0xFFU);
}

}  // namespace

size_t EcoFlowPacketCodec::encodePacket(const EcoFlowPacket& packet, uint8_t* out,
                                        size_t outCapacity) {
  const uint16_t payloadLen = packet.payloadLength;
  const size_t total = 18U + payloadLen + 2U;
  if (out == nullptr || outCapacity < total) {
    return 0;
  }
  if (payloadLen > 0 && packet.payload == nullptr) {
    return 0;
  }

  out[0] = kPacketPrefix;
  out[1] = packet.version;
  writeLe16(&out[2], payloadLen);
  out[4] = EcoFlowCrc::crc8Smbus(out, 4);
  out[5] = 0x0D;
  std::memcpy(&out[6], packet.seq, 4);
  out[10] = 0;
  out[11] = 0;
  out[12] = packet.src;
  out[13] = packet.dst;
  out[14] = packet.dsrc;
  out[15] = packet.ddst;
  out[16] = packet.cmdSet;
  out[17] = packet.cmdId;
  if (payloadLen > 0) {
    std::memcpy(&out[18], packet.payload, payloadLen);
  }
  const uint16_t crc = EcoFlowCrc::crc16Arc(out, 18U + payloadLen);
  writeLe16(&out[18U + payloadLen], crc);
  return total;
}

bool EcoFlowPacketCodec::decodePacket(const uint8_t* data, size_t length, EcoFlowPacket& out) {
  if (data == nullptr || length < kMinPacketBytes || data[0] != kPacketPrefix) {
    return false;
  }
  const uint8_t version = data[1];
  const uint16_t payloadLen = readLe16(&data[2]);
  const size_t total = 18U + payloadLen + 2U;
  if (length < total) {
    return false;
  }
  if (version == 3) {
    const uint16_t expectCrc = readLe16(&data[total - 2]);
    if (EcoFlowCrc::crc16Arc(data, total - 2) != expectCrc) {
      return false;
    }
  }
  if (EcoFlowCrc::crc8Smbus(data, 4) != data[4]) {
    return false;
  }

  out.version = version;
  out.productId = data[5];
  std::memcpy(out.seq, &data[6], 4);
  out.src = data[12];
  out.dst = data[13];
  out.dsrc = data[14];
  out.ddst = data[15];
  out.cmdSet = data[16];
  out.cmdId = data[17];
  out.payloadLength = payloadLen;
  out.payload = (payloadLen > 0) ? &data[18] : nullptr;
  return true;
}

uint16_t EcoFlowPacketCodec::deobfuscatePayload(uint8_t version, uint8_t seq0, uint8_t* payload,
                                                uint16_t payloadLen) {
  if (payload == nullptr || payloadLen == 0) {
    return 0;
  }
  if (seq0 != 0) {
    for (uint16_t i = 0; i < payloadLen; ++i) {
      payload[i] = static_cast<uint8_t>(payload[i] ^ seq0);
    }
  }
  // Version 19 telemetry often ends with 0xBB 0xBB sentinel.
  if (version == 19 && payloadLen >= 2 && payload[payloadLen - 2] == 0xBB &&
      payload[payloadLen - 1] == 0xBB) {
    payloadLen = static_cast<uint16_t>(payloadLen - 2);
  }
  return payloadLen;
}

size_t EcoFlowPacketCodec::encodeEncFrame(const EcoFlowEncFrame& frame, uint8_t* out,
                                          size_t outCapacity) {
  const uint16_t payloadWithCrc = static_cast<uint16_t>(frame.payloadLength + 2U);
  const size_t total = 6U + payloadWithCrc;
  if (out == nullptr || outCapacity < total) {
    return 0;
  }
  if (frame.payloadLength > 0 && frame.payload == nullptr) {
    return 0;
  }

  out[0] = kEncPrefix0;
  out[1] = kEncPrefix1;
  out[2] = static_cast<uint8_t>((frame.frameType & 0x0FU) << 4);
  out[3] = 0x01;
  writeLe16(&out[4], payloadWithCrc);
  if (frame.payloadLength > 0) {
    std::memcpy(&out[6], frame.payload, frame.payloadLength);
  }
  const uint16_t crc = EcoFlowCrc::crc16Arc(out, 6U + frame.payloadLength);
  writeLe16(&out[6U + frame.payloadLength], crc);
  return total;
}

size_t EcoFlowPacketCodec::decodeEncFrame(const uint8_t* data, size_t length, EcoFlowEncFrame& out) {
  if (data == nullptr || length < kMinEncBytes) {
    return 0;
  }
  if (data[0] != kEncPrefix0 || data[1] != kEncPrefix1) {
    return 0;
  }
  const uint16_t payloadWithCrc = readLe16(&data[4]);
  const size_t total = 6U + payloadWithCrc;
  if (length < total || payloadWithCrc < 2) {
    return 0;
  }
  const uint16_t payloadLen = static_cast<uint16_t>(payloadWithCrc - 2U);
  const uint16_t expectCrc = readLe16(&data[6U + payloadLen]);
  if (EcoFlowCrc::crc16Arc(data, 6U + payloadLen) != expectCrc) {
    return 0;
  }

  out.frameType = static_cast<uint8_t>((data[2] >> 4) & 0x0FU);
  out.payloadType = 0;
  out.payloadLength = payloadLen;
  out.payload = (payloadLen > 0) ? &data[6] : nullptr;
  return total;
}

}  // namespace ecoflow::protocol
