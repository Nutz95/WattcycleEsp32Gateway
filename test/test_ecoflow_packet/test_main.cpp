#include <unity.h>

#include "EcoFlow/Protocol/EcoFlowCrc.h"
#include "EcoFlow/Protocol/EcoFlowPacketCodec.h"

using ecoflow::protocol::EcoFlowCrc;
using ecoflow::protocol::EcoFlowEncFrame;
using ecoflow::protocol::EcoFlowPacket;
using ecoflow::protocol::EcoFlowPacketCodec;

void setUp() {}
void tearDown() {}

void test_crc8_smbus_empty() {
  TEST_ASSERT_EQUAL_UINT8(0x00, EcoFlowCrc::crc8Smbus(nullptr, 0));
}

void test_packet_roundtrip_empty_payload() {
  EcoFlowPacket packet{};
  packet.src = 0x21;
  packet.dst = 0x35;
  packet.cmdSet = 0x35;
  packet.cmdId = 0x89;
  packet.version = 3;
  packet.payload = nullptr;
  packet.payloadLength = 0;

  uint8_t buffer[64] = {};
  const size_t written = EcoFlowPacketCodec::encodePacket(packet, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_size_t(20, written);
  TEST_ASSERT_EQUAL_UINT8(0xAA, buffer[0]);

  EcoFlowPacket decoded{};
  TEST_ASSERT_TRUE(EcoFlowPacketCodec::decodePacket(buffer, written, decoded));
  TEST_ASSERT_EQUAL_UINT8(0x21, decoded.src);
  TEST_ASSERT_EQUAL_UINT8(0x35, decoded.dst);
  TEST_ASSERT_EQUAL_UINT8(0x35, decoded.cmdSet);
  TEST_ASSERT_EQUAL_UINT8(0x89, decoded.cmdId);
  TEST_ASSERT_EQUAL_UINT16(0, decoded.payloadLength);
}

void test_enc_frame_roundtrip() {
  const uint8_t payload[] = {0x01, 0x00, 0xAA, 0xBB};
  EcoFlowEncFrame frame{};
  frame.frameType = 0x00;
  frame.payload = payload;
  frame.payloadLength = sizeof(payload);

  uint8_t buffer[32] = {};
  const size_t written = EcoFlowPacketCodec::encodeEncFrame(frame, buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(written >= 8);
  TEST_ASSERT_EQUAL_UINT8(0x5A, buffer[0]);
  TEST_ASSERT_EQUAL_UINT8(0x5A, buffer[1]);

  EcoFlowEncFrame decoded{};
  const size_t consumed = EcoFlowPacketCodec::decodeEncFrame(buffer, written, decoded);
  TEST_ASSERT_EQUAL_size_t(written, consumed);
  TEST_ASSERT_EQUAL_UINT8(0x00, decoded.frameType);
  TEST_ASSERT_EQUAL_UINT16(sizeof(payload), decoded.payloadLength);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, decoded.payload, sizeof(payload));
}

void test_decode_rejects_bad_prefix() {
  uint8_t buffer[20] = {};
  EcoFlowPacket decoded{};
  TEST_ASSERT_FALSE(EcoFlowPacketCodec::decodePacket(buffer, sizeof(buffer), decoded));
}

void test_deobfuscate_xor_and_strip_bb() {
  // Wire payload = cleartext XOR seq0; cleartext ends with BB BB (v19).
  // clear: 08 00 1D BB BB  →  xor 0x86: 8E 86 9B 3D 3D
  uint8_t payload[] = {0x8E, 0x86, 0x9B, 0x3D, 0x3D};
  const uint16_t outLen =
      EcoFlowPacketCodec::deobfuscatePayload(19, 0x86, payload, sizeof(payload));
  TEST_ASSERT_EQUAL_UINT16(3, outLen);
  TEST_ASSERT_EQUAL_UINT8(0x08, payload[0]);
  TEST_ASSERT_EQUAL_UINT8(0x00, payload[1]);
  TEST_ASSERT_EQUAL_UINT8(0x1D, payload[2]);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_crc8_smbus_empty);
  RUN_TEST(test_packet_roundtrip_empty_payload);
  RUN_TEST(test_enc_frame_roundtrip);
  RUN_TEST(test_decode_rejects_bad_prefix);
  RUN_TEST(test_deobfuscate_xor_and_strip_bb);
  return UNITY_END();
}
