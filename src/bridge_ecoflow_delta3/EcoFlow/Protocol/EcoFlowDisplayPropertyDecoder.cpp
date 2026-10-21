#include "EcoFlow/Protocol/EcoFlowDisplayPropertyDecoder.h"

#include "EcoFlow/Protocol/EcoFlowProtobuf.h"

#include <cstddef>

namespace ecoflow::protocol {
namespace {

float absWatts(float value) {
  return value < 0.0f ? -value : value;
}

bool inSocRange(float value) {
  return value >= 0.0f && value <= 100.0f;
}

enum class Apply : uint8_t {
  AbsWatt,
  SignedWatt,
  SocCms,
  SocBms,
  UsbLane,
  Remain,
  RemainIfEmpty,
  FlowOn,
  SohCms,
  SohBms,
  CellTemp,
  Freq,
  Fan,
  Err,
  ChgCms,
  ChgBms,
  LimitChg,
  LimitDsg,
};

void setPreferredByte(bool& havePreferred, uint8_t& member, bool& dirty, uint8_t value,
                      bool preferred) {
  if (!preferred && havePreferred) {
    return;
  }
  member = value;
  if (preferred) {
    havePreferred = true;
  }
  dirty = true;
}

struct FieldSpec {
  uint32_t number;
  ProtoWireType wire;
  Apply apply;
  uint16_t arg;  // usb lane index, or offsetof(PowerStationTelemetry, …)
};

// Verified pd335 DisplayPropertyUpload field numbers (DELTA 3 / ha-ef-ble).
constexpr FieldSpec kFieldTable[] = {
    {EcoFlowDisplayPropertyDecoder::kPowGetQcusb1, ProtoWireType::Fixed32, Apply::UsbLane, 0},
    {EcoFlowDisplayPropertyDecoder::kPowGetQcusb2, ProtoWireType::Fixed32, Apply::UsbLane, 1},
    {EcoFlowDisplayPropertyDecoder::kPowGetTypec1, ProtoWireType::Fixed32, Apply::UsbLane, 2},
    {EcoFlowDisplayPropertyDecoder::kPowGetTypec2, ProtoWireType::Fixed32, Apply::UsbLane, 3},
    {EcoFlowDisplayPropertyDecoder::kPowInSumW, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, inSumW))},
    {EcoFlowDisplayPropertyDecoder::kPowOutSumW, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, outSumW))},
    {EcoFlowDisplayPropertyDecoder::kPowGet12v, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, dcOutputW))},
    {EcoFlowDisplayPropertyDecoder::kPowGetAcIn, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, acInputW))},
    {EcoFlowDisplayPropertyDecoder::kPowGetAcOut, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, acOutputW))},
    {EcoFlowDisplayPropertyDecoder::kPowGetPv, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, solarInputW))},
    {EcoFlowDisplayPropertyDecoder::kPowGetBms, ProtoWireType::Fixed32, Apply::SignedWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, bmsPowerW))},
    {EcoFlowDisplayPropertyDecoder::kBmsMinCellTemp, ProtoWireType::Varint, Apply::CellTemp,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, minCellTempC))},
    {EcoFlowDisplayPropertyDecoder::kBmsMaxCellTemp, ProtoWireType::Varint, Apply::CellTemp,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, maxCellTempC))},
    {EcoFlowDisplayPropertyDecoder::kBmsMinMosTemp, ProtoWireType::Varint, Apply::CellTemp,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, minMosTempC))},
    {EcoFlowDisplayPropertyDecoder::kBmsMaxMosTemp, ProtoWireType::Varint, Apply::CellTemp,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, maxMosTempC))},
    {EcoFlowDisplayPropertyDecoder::kCmsBattSoc, ProtoWireType::Fixed32, Apply::SocCms, 0},
    {EcoFlowDisplayPropertyDecoder::kBmsBattSoc, ProtoWireType::Fixed32, Apply::SocBms, 0},
    {EcoFlowDisplayPropertyDecoder::kCmsDsgRemTime, ProtoWireType::Varint, Apply::Remain, 0},
    {EcoFlowDisplayPropertyDecoder::kCmsChgRemTime, ProtoWireType::Varint, Apply::RemainIfEmpty, 0},
    {EcoFlowDisplayPropertyDecoder::kCmsBattSoh, ProtoWireType::Fixed32, Apply::SohCms, 0},
    {EcoFlowDisplayPropertyDecoder::kBmsBattSoh, ProtoWireType::Fixed32, Apply::SohBms, 0},
    {EcoFlowDisplayPropertyDecoder::kFlowInfoAcOut, ProtoWireType::Varint, Apply::FlowOn,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, acOutputOn))},
    {EcoFlowDisplayPropertyDecoder::kFlowInfo12v, ProtoWireType::Varint, Apply::FlowOn,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, dcOutputOn))},
    {EcoFlowDisplayPropertyDecoder::kFlowInfoAcIn, ProtoWireType::Varint, Apply::FlowOn,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, acInputOn))},
    {EcoFlowDisplayPropertyDecoder::kFlowInfoPv, ProtoWireType::Varint, Apply::FlowOn,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, pvInputOn))},
    {EcoFlowDisplayPropertyDecoder::kAcInFreq, ProtoWireType::Varint, Apply::Freq, 0},
    {EcoFlowDisplayPropertyDecoder::kPcsFanLevel, ProtoWireType::Varint, Apply::Fan, 0},
    {EcoFlowDisplayPropertyDecoder::kErrCode, ProtoWireType::Varint, Apply::Err, 0},
    {EcoFlowDisplayPropertyDecoder::kBmsErrCode, ProtoWireType::Varint, Apply::Err, 0},
    {EcoFlowDisplayPropertyDecoder::kCmsChgDsgState, ProtoWireType::Varint, Apply::ChgCms, 0},
    {EcoFlowDisplayPropertyDecoder::kBmsChgDsgState, ProtoWireType::Varint, Apply::ChgBms, 0},
    {EcoFlowDisplayPropertyDecoder::kCmsMaxChgSoc, ProtoWireType::Varint, Apply::LimitChg, 0},
    {EcoFlowDisplayPropertyDecoder::kCmsMinDsgSoc, ProtoWireType::Varint, Apply::LimitDsg, 0},
};

const FieldSpec* findSpec(uint32_t number, ProtoWireType wire) {
  for (const FieldSpec& spec : kFieldTable) {
    if (spec.number == number && spec.wire == wire) {
      return &spec;
    }
  }
  return nullptr;
}

int16_t* wattAt(models::PowerStationTelemetry& tele, uint16_t offset) {
  return reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(&tele) + offset);
}

bool* flagAt(models::PowerStationTelemetry& tele, uint16_t offset) {
  return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(&tele) + offset);
}

void applyCellTemp(models::PowerStationTelemetry& tele, bool& haveMaxCell, bool& dirty,
                   uint16_t arg, int32_t raw) {
  if (raw < EcoFlowDisplayPropertyDecoder::kMinCellTempC ||
      raw > EcoFlowDisplayPropertyDecoder::kMaxCellTempC) {
    return;
  }
  *wattAt(tele, arg) = static_cast<int16_t>(raw);
  tele.haveCellTemp = true;
  tele.haveTemperature = true;
  const auto maxCell =
      static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, maxCellTempC));
  const auto maxMos = static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, maxMosTempC));
  if (arg == maxCell) {
    tele.tempC = static_cast<int16_t>(raw);
    haveMaxCell = true;
  } else if (arg == maxMos && !haveMaxCell) {
    tele.tempC = static_cast<int16_t>(raw);
  }
  dirty = true;
}

void applySocLimit(models::PowerStationTelemetry& tele, bool& dirty, bool chargeLimit,
                   uint64_t varint) {
  if (varint > 100u) {
    return;
  }
  const auto limit = static_cast<uint8_t>(varint);
  if (chargeLimit) {
    tele.chargeLimitPercent = limit;
    tele.haveChargeLimit = true;
  } else {
    tele.dischargeLimitPercent = limit;
    tele.haveDischargeLimit = true;
  }
  dirty = true;
}

bool setByteFlag(uint8_t& member, bool& flag, bool& dirty, uint64_t varint) {
  if (varint > 255u) {
    return false;
  }
  member = static_cast<uint8_t>(varint);
  flag = true;
  dirty = true;
  return true;
}

}  // namespace

void EcoFlowDisplayPropertyDecoder::applyExtendedField(uint8_t applyKind, uint16_t arg,
                                                       const ProtoFieldView& field) {
  const auto kind = static_cast<Apply>(applyKind);
  auto& tele = snapshot_;
  switch (kind) {
    case Apply::SignedWatt:
      *wattAt(tele, arg) = clampRoundI16(field.float32);
      dirty_ = true;
      return;
    case Apply::SohCms:
    case Apply::SohBms:
      if (!inSocRange(field.float32)) {
        return;
      }
      setPreferredByte(haveCmsSoh_, tele.sohPercent, dirty_,
                       static_cast<uint8_t>(field.float32 + 0.5f), kind == Apply::SohCms);
      tele.haveSoh = true;
      return;
    case Apply::CellTemp:
      applyCellTemp(tele, haveMaxCell_, dirty_, arg,
                    static_cast<int32_t>(static_cast<uint32_t>(field.varint)));
      return;
    case Apply::Freq:
      setByteFlag(tele.acInFreqHz, tele.haveFreq, dirty_, field.varint);
      return;
    case Apply::Fan:
      setByteFlag(tele.fanLevel, tele.haveFan, dirty_, field.varint);
      return;
    case Apply::Err:
      tele.errCode = static_cast<uint32_t>(field.varint);
      tele.haveErr = true;
      dirty_ = true;
      return;
    case Apply::ChgCms:
    case Apply::ChgBms: {
      if (field.varint > 255u) {
        return;
      }
      setPreferredByte(haveCmsChg_, tele.chgDsgState, dirty_,
                       static_cast<uint8_t>(field.varint), kind == Apply::ChgCms);
      tele.haveChgState = true;
      return;
    }
    case Apply::LimitChg:
    case Apply::LimitDsg:
      applySocLimit(tele, dirty_, kind == Apply::LimitChg, field.varint);
      return;
    default:
      return;
  }
}

void applyDisplayField(EcoFlowDisplayPropertyDecoder& self, const ProtoFieldView& field) {
  const FieldSpec* spec = findSpec(field.number, field.wireType);
  if (spec == nullptr) {
    self.applyRuntime(field);
    return;
  }
  switch (spec->apply) {
    case Apply::AbsWatt:
      self.setAbsWattMember(*wattAt(self.snapshot_, spec->arg), field.float32);
      return;
    case Apply::SocCms:
      self.setSoc(field.float32, /*fromCms=*/true);
      return;
    case Apply::SocBms:
      self.setSoc(field.float32, /*fromCms=*/false);
      return;
    case Apply::UsbLane:
      self.setUsbLaneWatts(spec->arg, field.float32);
      return;
    case Apply::Remain:
      self.setRemainMinutes(static_cast<uint32_t>(field.varint), /*preferIfEmpty=*/false);
      return;
    case Apply::RemainIfEmpty:
      self.setRemainMinutes(static_cast<uint32_t>(field.varint), /*preferIfEmpty=*/true);
      return;
    case Apply::FlowOn:
      *flagAt(self.snapshot_, spec->arg) =
          EcoFlowDisplayPropertyDecoder::flowIsOn(static_cast<uint32_t>(field.varint));
      self.dirty_ = true;
      return;
    default:
      self.applyExtendedField(static_cast<uint8_t>(spec->apply), spec->arg, field);
      return;
  }
}

void EcoFlowDisplayPropertyDecoder::reset() {
  snapshot_ = models::PowerStationTelemetry{};
  for (size_t i = 0; i < kUsbLaneCount; ++i) {
    usbWatts_[i] = 0.0f;
    haveUsbWatts_[i] = false;
  }
  haveCmsSoc_ = false;
  haveCmsSoh_ = false;
  haveCmsChg_ = false;
  haveMaxCell_ = false;
  haveCmsVolts_ = false;
  haveCmsAmps_ = false;
  dirty_ = false;
}

bool EcoFlowDisplayPropertyDecoder::flowIsOn(uint32_t value) {
  return (value & 0x2U) != 0;
}

int16_t EcoFlowDisplayPropertyDecoder::clampRoundI16(float value) {
  if (value > kI16AbsMax) {
    return kI16Clamp;
  }
  if (value < -kI16AbsMax) {
    return static_cast<int16_t>(-kI16Clamp);
  }
  const float rounded = value >= 0.0f ? (value + 0.5f) : (value - 0.5f);
  return static_cast<int16_t>(rounded);
}

int16_t EcoFlowDisplayPropertyDecoder::wattsToI16(float wattsAbs) {
  return clampRoundI16(wattsAbs);
}

bool EcoFlowDisplayPropertyDecoder::onField(const ProtoFieldView& field, void* user) {
  auto* self = static_cast<EcoFlowDisplayPropertyDecoder*>(user);
  if (self != nullptr) {
    applyDisplayField(*self, field);
  }
  return true;
}

void EcoFlowDisplayPropertyDecoder::setAbsWattMember(int16_t& member, float value) {
  member = wattsToI16(absWatts(value));
  dirty_ = true;
}

void EcoFlowDisplayPropertyDecoder::setSoc(float value, bool fromCms) {
  if (!inSocRange(value)) {
    return;
  }
  if (!fromCms && haveCmsSoc_) {
    return;
  }
  snapshot_.socPercent = static_cast<uint8_t>(value + 0.5f);
  if (fromCms) {
    haveCmsSoc_ = true;
  }
  dirty_ = true;
}

void EcoFlowDisplayPropertyDecoder::setUsbLaneWatts(size_t lane, float value) {
  if (lane >= kUsbLaneCount) {
    return;
  }
  usbWatts_[lane] = absWatts(value);
  haveUsbWatts_[lane] = true;
  dirty_ = true;
}

void EcoFlowDisplayPropertyDecoder::setRemainMinutes(uint32_t value, bool preferIfEmpty) {
  if (value >= 0xFFFFU) {
    return;
  }
  if (preferIfEmpty && snapshot_.remainMinutes != 0) {
    return;
  }
  snapshot_.remainMinutes = static_cast<uint16_t>(value);
  dirty_ = true;
}

void EcoFlowDisplayPropertyDecoder::publish(models::PowerStationTelemetry& out) const {
  out = snapshot_;
  float usbSum = 0.0f;
  bool haveUsb = false;
  for (size_t i = 0; i < kUsbLaneCount; ++i) {
    if (!haveUsbWatts_[i]) {
      continue;
    }
    usbSum += usbWatts_[i];
    haveUsb = true;
  }
  if (haveUsb) {
    out.usbOutputW = wattsToI16(usbSum);
    // flow_info USB field numbers are not verified on this unit yet.
    // Presence of any USB-lane power field means the port group is reporting
    // (covers tiny ESP loads that stay near 0 W). Latches until reset().
    out.usbOutputOn = true;
  }
  out.valid = dirty_;
}

bool EcoFlowDisplayPropertyDecoder::merge(const uint8_t* payload, size_t length,
                                          models::PowerStationTelemetry& out) {
  if (payload == nullptr || length == 0) {
    return false;
  }
  if (!EcoFlowProtobuf::walk(payload, length, onField, this)) {
    return false;
  }
  publish(out);
  return dirty_;
}

}  // namespace ecoflow::protocol
