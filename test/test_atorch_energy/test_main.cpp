#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Meter/Protocol/AtorchFrameParser.h"

using xt369p::meter::AtorchFrameParser;

void test_esphome_dc_scale_times_ten() {
  // ESPHome sample: raw energy 32 → 320 Wh; Ah×V ≈ 12.36 * 28.2 ≈ 348.
  const float wh = AtorchFrameParser::resolveEnergyWh(32u, 12.36f, 28.2f);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 320.0f, wh);
}

void test_xt369p_small_raw_uses_times_ten_not_div_100() {
  // Dashboard bug: raw=1 with /100 → 0.010 Wh; *10 → 10 Wh near LCD ~13 Wh.
  const float wh = AtorchFrameParser::resolveEnergyWh(1u, 0.542f, 33.2f);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, wh);
}

void test_nicelabs_div_100_when_closer_to_ahv() {
  // raw=1318 → /100 = 13.18 Wh; Ah×V ≈ 17.9.
  const float wh = AtorchFrameParser::resolveEnergyWh(1318u, 0.542f, 33.2f);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 13.18f, wh);
}

void test_milliwh_div_1000_when_closer_to_ahv() {
  // raw=13184 → /1000 = 13.184 Wh.
  const float wh = AtorchFrameParser::resolveEnergyWh(13184u, 0.542f, 33.2f);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 13.184f, wh);
}

void test_zero_raw_falls_back_to_ah_times_v() {
  const float wh = AtorchFrameParser::resolveEnergyWh(0u, 0.542f, 33.2f);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.542f * 33.2f, wh);
}

void test_feed_dc_frame_esphome_sample() {
  // From esphome-atorch-dl24 DC sample (CRC may not match XT369P variant).
  const uint8_t frame[] = {
      0xFF, 0x55, 0x01, 0x02, 0x00, 0x01, 0x1A, 0x00, 0x00, 0x3C, 0x00, 0x04, 0xD4,
      0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26,
      0x00, 0x59, 0x0D, 0x36, 0x3C, 0x00, 0x00, 0x00, 0x00, 0xF0};
  AtorchFrameParser parser;
  xt369p::meter::WattmeterTelemetry out{};
  TEST_ASSERT_TRUE(parser.feed(frame, sizeof(frame), out));
  TEST_ASSERT_TRUE(out.valid);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 28.2f, out.voltageV);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 12.36f, out.capacityAh);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 320.0f, out.energyWh);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_esphome_dc_scale_times_ten);
  RUN_TEST(test_xt369p_small_raw_uses_times_ten_not_div_100);
  RUN_TEST(test_nicelabs_div_100_when_closer_to_ahv);
  RUN_TEST(test_milliwh_div_1000_when_closer_to_ahv);
  RUN_TEST(test_zero_raw_falls_back_to_ah_times_v);
  RUN_TEST(test_feed_dc_frame_esphome_sample);
  return UNITY_END();
}
