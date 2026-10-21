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

void test_station_detail_fields() {
  std::vector<uint8_t> payload;
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kCmsBattSoh, 97.4f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kBmsBattSoh, 50.0f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kPowInSumW, 180.2f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kPowOutSumW, 40.0f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kPowGetBms, -12.6f);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kBmsMinCellTemp,
                    static_cast<uint32_t>(static_cast<int32_t>(-2)));
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kBmsMaxCellTemp, 28);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kBmsMaxMosTemp, 31);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kAcInFreq, 50);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kPcsFanLevel, 1);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kCmsChgDsgState, 2);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kBmsChgDsgState, 1);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kCmsMaxChgSoc, 90);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kCmsMinDsgSoc, 10);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kFlowInfoPv, 2);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kErrCode, 0);

  EcoFlowDisplayPropertyDecoder decoder;
  PowerStationTelemetry tele{};
  TEST_ASSERT_TRUE(decoder.merge(payload.data(), payload.size(), tele));
  TEST_ASSERT_TRUE(tele.haveSoh);
  TEST_ASSERT_EQUAL_UINT8(97, tele.sohPercent);
  TEST_ASSERT_EQUAL_INT16(180, tele.inSumW);
  TEST_ASSERT_EQUAL_INT16(40, tele.outSumW);
  TEST_ASSERT_EQUAL_INT16(-13, tele.bmsPowerW);
  TEST_ASSERT_TRUE(tele.haveCellTemp);
  TEST_ASSERT_TRUE(tele.haveTemperature);
  TEST_ASSERT_EQUAL_INT16(-2, tele.minCellTempC);
  TEST_ASSERT_EQUAL_INT16(28, tele.maxCellTempC);
  TEST_ASSERT_EQUAL_INT16(28, tele.tempC);
  TEST_ASSERT_EQUAL_INT16(31, tele.maxMosTempC);
  TEST_ASSERT_EQUAL_UINT8(50, tele.acInFreqHz);
  TEST_ASSERT_EQUAL_UINT8(1, tele.fanLevel);
  TEST_ASSERT_EQUAL_UINT8(2, tele.chgDsgState);
  TEST_ASSERT_EQUAL_UINT8(90, tele.chargeLimitPercent);
  TEST_ASSERT_EQUAL_UINT8(10, tele.dischargeLimitPercent);
  TEST_ASSERT_TRUE(tele.pvInputOn);
  TEST_ASSERT_TRUE(tele.haveErr);
  TEST_ASSERT_EQUAL_UINT32(0, tele.errCode);
}

void test_runtime_volts_amps_and_cells() {
  std::vector<uint8_t> payload;
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kAcInVolts, 230.4f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kBmsVolts, 51.25f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kBmsAmps, -0.92f);
  appendFloatField(payload, EcoFlowDisplayPropertyDecoder::kCmsVolts, 51.5f);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kBmsMinCellMv, 3180);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kBmsMaxCellMv, 3330);
  appendVarintField(payload, EcoFlowDisplayPropertyDecoder::kBmsFullCapMah, 20000);

  EcoFlowDisplayPropertyDecoder decoder;
  PowerStationTelemetry tele{};
  TEST_ASSERT_TRUE(decoder.merge(payload.data(), payload.size(), tele));
  TEST_ASSERT_TRUE(tele.haveAcInElec);
  TEST_ASSERT_EQUAL_INT16(2304, tele.acInVoltsDeci);
  TEST_ASSERT_TRUE(tele.haveBmsElec);
  TEST_ASSERT_EQUAL_INT16(5150, tele.bmsVoltsCenti);
  TEST_ASSERT_EQUAL_INT16(-92, tele.bmsCentiamps);
  TEST_ASSERT_TRUE(tele.haveCellVolts);
  TEST_ASSERT_EQUAL_UINT16(3180, tele.minCellMv);
  TEST_ASSERT_EQUAL_UINT16(3330, tele.maxCellMv);
  TEST_ASSERT_EQUAL_UINT16(20000, tele.fullCapMah);
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
  RUN_TEST(test_station_detail_fields);
  RUN_TEST(test_runtime_volts_amps_and_cells);
  RUN_TEST(test_usb_zero_watts_still_on_when_lane_present);
  return UNITY_END();
}
