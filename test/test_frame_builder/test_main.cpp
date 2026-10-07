#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Bms/Protocol/FrameBuilder.h"
#include "Bms/Protocol/ModbusCrc16.h"

using wattcycle::bms::FrameBuilder;
using wattcycle::bms::ModbusCrc16;
using wattcycle::bms::kFrameHead;
using wattcycle::bms::kFrameTail;

void test_build_analog_read_frame() {
  uint8_t frame[16] = {};
  const size_t length = FrameBuilder::buildAnalogQuantityRead(kFrameHead, frame, sizeof(frame));
  TEST_ASSERT_EQUAL(11, length);
  TEST_ASSERT_EQUAL_HEX8(kFrameHead, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x8C, frame[5]);
  TEST_ASSERT_EQUAL_HEX8(kFrameTail, frame[10]);
  TEST_ASSERT_TRUE(ModbusCrc16::verifyFrame(frame, length));
}

void test_build_rejects_small_buffer() {
  uint8_t frame[4] = {};
  TEST_ASSERT_EQUAL(0, FrameBuilder::buildAnalogQuantityRead(kFrameHead, frame, sizeof(frame)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_build_analog_read_frame);
  RUN_TEST(test_build_rejects_small_buffer);
  return UNITY_END();
}
