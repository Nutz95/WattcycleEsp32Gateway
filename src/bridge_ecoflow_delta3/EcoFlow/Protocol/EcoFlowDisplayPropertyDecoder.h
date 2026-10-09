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
  static constexpr uint32_t kFlowInfo12v = 33;
  static constexpr uint32_t kPowGet12v = 37;
  static constexpr uint32_t kPowGetAcIn = 54;
  static constexpr uint32_t kBmsBattSoc = 242;
  static constexpr uint32_t kCmsBattSoc = 262;
  static constexpr uint32_t kCmsDsgRemTime = 268;
  static constexpr uint32_t kCmsChgRemTime = 269;
  static constexpr uint32_t kPowGetPv = 361;
  static constexpr uint32_t kFlowInfoAcOut = 367;
  static constexpr uint32_t kPowGetAcOut = 368;

  /// Clears merged snapshot and USB scratch state.
  void reset();
  /// Merge one protobuf payload; returns true if any watched field changed.
  bool merge(const uint8_t* payload, size_t length, models::PowerStationTelemetry& out);

  /// EcoFlow flow_info_*: bit1 set means the port is enabled (2/14/… on, 4/… off).
  static bool flowIsOn(uint32_t value);

 private:
  friend void applyDisplayField(EcoFlowDisplayPropertyDecoder& self, const ProtoFieldView& field);

  static constexpr size_t kUsbLaneCount = 4;

  static bool onField(const ProtoFieldView& field, void* user);
  void setAbsWattMember(int16_t& member, float value);
  void setSoc(float value, bool fromCms);
  void setUsbLaneWatts(size_t lane, float value);
  void setRemainMinutes(uint32_t value, bool preferIfEmpty);
  void publish(models::PowerStationTelemetry& out) const;
  static int16_t wattsToI16(float wattsAbs);

  models::PowerStationTelemetry snapshot_{};
  float usbWatts_[kUsbLaneCount] = {};
  bool haveUsbWatts_[kUsbLaneCount] = {};
  bool haveCmsSoc_ = false;
  bool dirty_ = false;
};

}  // namespace ecoflow::protocol
