#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Bms/Protocol/FrameParser.h"

using wattcycle::bms::FrameParser;

void test_expected_length_from_header() {
  const uint8_t header[] = {0x7E, 0x00, 0x01, 0x03, 0x00, 0x8C, 0x00, 0x14};
  TEST_ASSERT_EQUAL(31, FrameParser::expectedResponseLength(header, sizeof(header)));
}

void test_parse_valid_minimal_frame() {
  // data_len = 0, empty payload, CRC over first 8 bytes of empty-data response shape
  // Build: head ver addr func start(2) len(2)=0 crc(2) tail
  const uint8_t frame[] = {0x7E, 0x00, 0x01, 0x03, 0x00, 0x8C, 0x00, 0x00, 0x99, 0x42, 0x0D};
  const auto parsed = FrameParser::parse(frame, sizeof(frame));
  TEST_ASSERT_TRUE(parsed.valid);
  TEST_ASSERT_EQUAL_HEX16(0x008C, parsed.startAddress);
  TEST_ASSERT_EQUAL(0, parsed.dataLength);
}

void test_parse_rejects_bad_tail() {
  const uint8_t frame[] = {0x7E, 0x00, 0x01, 0x03, 0x00, 0x8C, 0x00, 0x00, 0x99, 0x42, 0x00};
  TEST_ASSERT_FALSE(FrameParser::parse(frame, sizeof(frame)).valid);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_expected_length_from_header);
  RUN_TEST(test_parse_valid_minimal_frame);
  RUN_TEST(test_parse_rejects_bad_tail);
  return UNITY_END();
}
