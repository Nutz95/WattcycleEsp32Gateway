#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Bms/Protocol/FrameBuilder.h"
#include "Bms/Protocol/ModbusCrc16.h"

using wattcycle::bms::FrameBuilder;
using wattcycle::bms::ModbusCrc16;
using wattcycle::bms::kDpChargeDischargeSwitch;
using wattcycle::bms::kFrameHead;
using wattcycle::bms::kFrameTail;
using wattcycle::bms::kFuncWrite;

void test_build_analog_read_frame() {
  uint8_t frame[16] = {};
  const size_t length = FrameBuilder::buildAnalogQuantityRead(kFrameHead, frame, sizeof(frame));
  TEST_ASSERT_EQUAL(11, length);
  TEST_ASSERT_EQUAL_HEX8(kFrameHead, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x8C, frame[5]);
  TEST_ASSERT_EQUAL_HEX8(kFrameTail, frame[10]);
  TEST_ASSERT_TRUE(ModbusCrc16::verifyFrame(frame, length));
}

void test_build_charge_switch_write_copies_payload() {
  const uint8_t payload[] = {0xA5, 0x5A};
  uint8_t frame[16] = {};
  const size_t length = FrameBuilder::buildWriteFrame(kDpChargeDischargeSwitch, payload, sizeof(payload),
                                                      kFrameHead, frame, sizeof(frame));
  TEST_ASSERT_EQUAL(13, length);
  TEST_ASSERT_EQUAL_HEX8(kFuncWrite, frame[3]);
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[4]);
  TEST_ASSERT_EQUAL_HEX8(0x82, frame[5]);
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[6]);
  TEST_ASSERT_EQUAL_HEX8(0x02, frame[7]);
  TEST_ASSERT_EQUAL_HEX8(0xA5, frame[8]);
  TEST_ASSERT_EQUAL_HEX8(0x5A, frame[9]);
  TEST_ASSERT_EQUAL_HEX8(kFrameTail, frame[12]);
  TEST_ASSERT_TRUE(ModbusCrc16::verifyFrame(frame, length));
}

void test_build_rejects_small_buffer() {
  uint8_t frame[4] = {};
  TEST_ASSERT_EQUAL(0, FrameBuilder::buildAnalogQuantityRead(kFrameHead, frame, sizeof(frame)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_build_analog_read_frame);
  RUN_TEST(test_build_charge_switch_write_copies_payload);
  RUN_TEST(test_build_rejects_small_buffer);
  return UNITY_END();
}
