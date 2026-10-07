#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Bms/Protocol/AnalogQuantityParser.h"

using wattcycle::bms::AnalogQuantityParser;
using wattcycle::bms::BatteryTelemetry;

void test_parse_signed_current_decimal_negative() {
  // sign=1 decimal=1 raw=123 -> -12.3 A
  const float current = AnalogQuantityParser::parseSignedCurrent(0xC0 | 0x00, 123);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -12.3f, current);
}

void test_parse_two_cell_pack() {
  // cellCount=2, cells 3.300V/3.310V, tempCount=2, MOS/PCB 25C,
  // current 10.0A, pack 51.20V, rem 80.0, tot 100.0, cycles 12, design 100.0, soc 80
  const uint8_t payload[] = {
      0x02, 0x0C, 0xE4, 0x0C, 0xEE, 0x02, 0x0B, 0xA4, 0x0B, 0xA4, 0x40, 0x64,
      0x14, 0x00, 0x03, 0x20, 0x03, 0xE8, 0x00, 0x0C, 0x03, 0xE8, 0x00, 0x50};

  BatteryTelemetry telemetry;
  TEST_ASSERT_TRUE(AnalogQuantityParser::parse(payload, sizeof(payload), telemetry));
  TEST_ASSERT_TRUE(telemetry.valid);
  TEST_ASSERT_EQUAL(2, telemetry.cellCount);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.300f, telemetry.cellVoltages[0]);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.310f, telemetry.cellVoltages[1]);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 25.0f, telemetry.mosTemperatureC);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 10.0f, telemetry.currentAmps);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 51.20f, telemetry.moduleVoltage);
  TEST_ASSERT_EQUAL(80, telemetry.stateOfChargePercent);
  TEST_ASSERT_EQUAL(12, telemetry.cycleNumber);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_signed_current_decimal_negative);
  RUN_TEST(test_parse_two_cell_pack);
  return UNITY_END();
}
