#include <unity.h>

#include "EcoFlow/Protocol/EcoFlowDisplayPropertyDecoder.h"

#include <cstring>
#include <vector>

using ecoflow::models::PowerStationTelemetry;
using ecoflow::protocol::EcoFlowDisplayPropertyDecoder;

namespace {

void appendVarint(std::vector<uint8_t>& out, uint64_t value) {
  while (value >= 0x80) {
    out.push_back(static_cast<uint8_t>((value & 0x7FU) | 0x80U));
    value >>= 7;
  }
  out.push_back(static_cast<uint8_t>(value));
}

void appendFloatField(std::vector<uint8_t>& out, uint32_t number, float value) {
  appendVarint(out, (static_cast<uint64_t>(number) << 3) | 5U);
  uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  out.push_back(static_cast<uint8_t>(bits & 0xFFU));
  out.push_back(static_cast<uint8_t>((bits >> 8) & 0xFFU));
  out.push_back(static_cast<uint8_t>((bits >> 16) & 0xFFU));
  out.push_back(static_cast<uint8_t>((bits >> 24) & 0xFFU));
}

void appendVarintField(std::vector<uint8_t>& out, uint32_t number, uint32_t value) {
  appendVarint(out, (static_cast<uint64_t>(number) << 3) | 0U);
  appendVarint(out, value);
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_flow_is_on_masks() {
  TEST_ASSERT_TRUE(EcoFlowDisplayPropertyDecoder::flowIsOn(14));
  TEST_ASSERT_TRUE(EcoFlowDisplayPropertyDecoder::flowIsOn(2));
  TEST_ASSERT_FALSE(EcoFlowDisplayPropertyDecoder::flowIsOn(4));
  TEST_ASSERT_FALSE(EcoFlowDisplayPropertyDecoder::flowIsOn(0));
}

void test_merge_soc_and_ac_watts() {
  std::vector<uint8_t> payload;
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kCmsBattSoc, 75.0f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kPowGetAcIn, 46.3f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kPowGetAcOut, -12.0f);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kFlowInfoAcOut, 14);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kFlowInfo12v, 4);

  EcoFlowDisplayPropertyDecoder decoder;
  PowerStationTelemetry tele{};
  TEST_ASSERT_TRUE(decoder.merge(payload.data(), payload.size(), tele));
  TEST_ASSERT_EQUAL_UINT8(75, tele.socPercent);
  TEST_ASSERT_EQUAL_INT16(46, tele.acInputW);
  TEST_ASSERT_EQUAL_INT16(12, tele.acOutputW);
  TEST_ASSERT_TRUE(tele.acOutputOn);
  TEST_ASSERT_FALSE(tele.dcOutputOn);
  TEST_ASSERT_FALSE(tele.haveTemperature);
}

void test_partial_frame_keeps_soc() {
  EcoFlowDisplayPropertyDecoder decoder;
  PowerStationTelemetry tele{};

  std::vector<uint8_t> first;
  appendFloatField(first, EcoFlowDisplayPropertyDecoder::kCmsBattSoc, 80.0f);
  TEST_ASSERT_TRUE(decoder.merge(first.data(), first.size(), tele));
  TEST_ASSERT_EQUAL_UINT8(80, tele.socPercent);

  std::vector<uint8_t> second;
  appendFloatField(second, EcoFlowDisplayPropertyDecoder::kPowGetAcIn, 10.0f);
  TEST_ASSERT_TRUE(decoder.merge(second.data(), second.size(), tele));
  TEST_ASSERT_EQUAL_UINT8(80, tele.socPercent);
  TEST_ASSERT_EQUAL_INT16(10, tele.acInputW);
}

void test_usb_ports_sum_and_on() {
  std::vector<uint8_t> payload;
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kPowGetQcusb1, -2.0f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kPowGetTypec2, -3.5f);

  EcoFlowDisplayPropertyDecoder decoder;
  PowerStationTelemetry tele{};
  TEST_ASSERT_TRUE(decoder.merge(payload.data(), payload.size(), tele));
  TEST_ASSERT_EQUAL_INT16(6, tele.usbOutputW);
  TEST_ASSERT_TRUE(tele.usbOutputOn);
}

void test_usb_zero_watts_still_on_when_lane_present() {
  std::vector<uint8_t> payload;
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kPowGetTypec1, 0.0f);

  EcoFlowDisplayPropertyDecoder decoder;
  PowerStationTelemetry tele{};
  TEST_ASSERT_TRUE(decoder.merge(payload.data(), payload.size(), tele));
  TEST_ASSERT_EQUAL_INT16(0, tele.usbOutputW);
  TEST_ASSERT_TRUE(tele.usbOutputOn);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_flow_is_on_masks);
  RUN_TEST(test_merge_soc_and_ac_watts);
  RUN_TEST(test_partial_frame_keeps_soc);
  RUN_TEST(test_usb_ports_sum_and_on);
  RUN_TEST(test_usb_zero_watts_still_on_when_lane_present);
  return UNITY_END();
}
