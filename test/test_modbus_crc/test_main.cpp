#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Bms/Protocol/ModbusCrc16.h"

using wattcycle::bms::ModbusCrc16;

void test_crc_empty_payload_is_ffff() {
  const uint8_t empty[] = {0};
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, ModbusCrc16::compute(empty, 0));
}

void test_crc_analog_read_request() {
  const uint8_t payload[] = {0x7E, 0x00, 0x01, 0x03, 0x00, 0x8C, 0x00, 0x00};
  TEST_ASSERT_EQUAL_HEX16(0x9942, ModbusCrc16::compute(payload, sizeof(payload)));
}

void test_verify_complete_frame() {
  const uint8_t frame[] = {0x7E, 0x00, 0x01, 0x03, 0x00, 0x8C, 0x00, 0x00, 0x99, 0x42, 0x0D};
  TEST_ASSERT_TRUE(ModbusCrc16::verifyFrame(frame, sizeof(frame)));
}

void test_verify_rejects_bad_crc() {
  const uint8_t frame[] = {0x7E, 0x00, 0x01, 0x03, 0x00, 0x8C, 0x00, 0x00, 0x00, 0x00, 0x0D};
  TEST_ASSERT_FALSE(ModbusCrc16::verifyFrame(frame, sizeof(frame)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_crc_empty_payload_is_ffff);
  RUN_TEST(test_crc_analog_read_request);
  RUN_TEST(test_verify_complete_frame);
  RUN_TEST(test_verify_rejects_bad_crc);
  return UNITY_END();
}
