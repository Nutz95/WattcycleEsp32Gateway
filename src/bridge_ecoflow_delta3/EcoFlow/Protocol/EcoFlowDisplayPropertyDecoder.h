#pragma once

#include "EcoFlow/Models/PowerStationTelemetry.h"
#include "EcoFlow/Protocol/ProtoFieldView.h"

#include <cstddef>
#include <cstdint>

namespace ecoflow::protocol {

/// Merges pd335 `DisplayPropertyUpload` (cmdSet=0xFE, cmdId=0x15) into a snapshot.
/// Partial frames only update fields that are present.
class EcoFlowDisplayPropertyDecoder {
 public:
  // Field numbers verified on-wire for DELTA 3 (alanstrok / ha-ef-ble pd335 maps).
  static constexpr uint32_t kPowGetQcusb1 = 9;
  static constexpr uint32_t kPowGetQcusb2 = 10;
  static constexpr uint32_t kPowGetTypec1 = 11;
  static constexpr uint32_t kPowGetTypec2 = 12;
  static constexpr uint32_t kErrCode = 1;
  static constexpr uint32_t kPowInSumW = 3;
  static constexpr uint32_t kPowOutSumW = 4;
  static constexpr uint32_t kPcsFanLevel = 30;
  static constexpr uint32_t kFlowInfo12v = 33;
  static constexpr uint32_t kPowGet12v = 37;
  static constexpr uint32_t kFlowInfoAcIn = 47;
  static constexpr uint32_t kPowGetAcIn = 54;
  static constexpr uint32_t kAcInFreq = 62;
  static constexpr uint32_t kBmsErrCode = 140;
  static constexpr uint32_t kPowGetBms = 158;
  static constexpr uint32_t kBmsBattSoc = 242;
  static constexpr uint32_t kBmsBattSoh = 243;
  static constexpr uint32_t kBmsMinCellTemp = 258;
  static constexpr uint32_t kBmsMaxCellTemp = 259;
  static constexpr uint32_t kBmsMinMosTemp = 260;
  static constexpr uint32_t kBmsMaxMosTemp = 261;
  static constexpr uint32_t kCmsBattSoc = 262;
  static constexpr uint32_t kCmsBattSoh = 263;
  static constexpr uint32_t kCmsDsgRemTime = 268;
  static constexpr uint32_t kCmsChgRemTime = 269;
  static constexpr uint32_t kCmsMaxChgSoc = 270;
  static constexpr uint32_t kCmsMinDsgSoc = 271;
  static constexpr uint32_t kBmsChgDsgState = 281;
  static constexpr uint32_t kCmsChgDsgState = 282;
  static constexpr uint32_t kFlowInfoPv = 360;
  static constexpr uint32_t kPowGetPv = 361;
  static constexpr uint32_t kFlowInfoAcOut = 367;
  static constexpr uint32_t kPowGetAcOut = 368;
  // RuntimePropertyUpload property ids (same pd map; cmd 0xFE/0x16).
  static constexpr uint32_t kTempPcsDc = 26;
  static constexpr uint32_t kTempPcsAc = 27;
  static constexpr uint32_t kAcOutVolts = 67;
  static constexpr uint32_t kAcInVolts = 68;
  static constexpr uint32_t kAcInAmps = 223;
  static constexpr uint32_t kAcOutAmps = 224;
  static constexpr uint32_t kBmsVolts = 244;
  static constexpr uint32_t kBmsAmps = 245;
  static constexpr uint32_t kBmsFullCapMah = 247;
  static constexpr uint32_t kBmsRemainCapMah = 249;
  static constexpr uint32_t kBmsMinCellMv = 256;
  static constexpr uint32_t kBmsMaxCellMv = 257;
  static constexpr uint32_t kCmsVolts = 264;
  static constexpr uint32_t kCmsAmps = 265;
  /// int16 saturation for watt and scaled electrical readings.
  static constexpr float kI16AbsMax = 32000.0f;
  static constexpr int16_t kI16Clamp = 32000;
  /// Storage scales: AC volts are deci-volts; amps and pack volts are centi-units.
  static constexpr int kDeci = 10;
  static constexpr int kCenti = 100;
  static constexpr uint64_t kMinCellMv = 1500;
  static constexpr uint64_t kMaxCellMv = 4500;
  static constexpr uint64_t kMaxCapMah = 65000;
  static constexpr int32_t kMinCellTempC = -40;
  static constexpr int32_t kMaxCellTempC = 125;

  /// Clears merged snapshot and USB scratch state.
  void reset();
  /// Merge one protobuf payload; returns true if any watched field changed.
  bool merge(const uint8_t* payload, size_t length, models::PowerStationTelemetry& out);

  /// EcoFlow flow_info_*: bit1 set means the port is enabled (2/14/… on, 4/… off).
  static bool flowIsOn(uint32_t value);
  /// Round a scaled reading into int16, saturating at ±kI16Clamp.
  static int16_t clampRoundI16(float value);

 private:
  friend void applyDisplayField(EcoFlowDisplayPropertyDecoder& self, const ProtoFieldView& field);

  static constexpr size_t kUsbLaneCount = 4;

  static bool onField(const ProtoFieldView& field, void* user);
  void setAbsWattMember(int16_t& member, float value);
  void setSoc(float value, bool fromCms);
  void setUsbLaneWatts(size_t lane, float value);
  void setRemainMinutes(uint32_t value, bool preferIfEmpty);
  void publish(models::PowerStationTelemetry& out) const;
  /// Apply one RuntimePropertyUpload field onto the merged snapshot.
  void applyRuntime(const ProtoFieldView& field);
  void applyExtendedField(uint8_t kind, uint16_t arg, const ProtoFieldView& field);
  static int16_t wattsToI16(float wattsAbs);

  models::PowerStationTelemetry snapshot_{};
  float usbWatts_[kUsbLaneCount] = {};
  bool haveUsbWatts_[kUsbLaneCount] = {};
  bool haveCmsSoc_ = false;
  bool haveCmsSoh_ = false;
  bool haveCmsChg_ = false;
  bool haveMaxCell_ = false;
  bool haveCmsVolts_ = false;
  bool haveCmsAmps_ = false;
  bool dirty_ = false;
};

}  // namespace ecoflow::protocol
