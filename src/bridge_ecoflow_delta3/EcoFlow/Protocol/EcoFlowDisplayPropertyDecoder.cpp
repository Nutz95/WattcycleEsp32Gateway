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
  SocCms,
  SocBms,
  UsbLane,
  Remain,
  RemainIfEmpty,
  FlowOn,
};

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
    {EcoFlowDisplayPropertyDecoder::kPowGet12v, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, dcOutputW))},
    {EcoFlowDisplayPropertyDecoder::kPowGetAcIn, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, acInputW))},
    {EcoFlowDisplayPropertyDecoder::kPowGetAcOut, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, acOutputW))},
    {EcoFlowDisplayPropertyDecoder::kPowGetPv, ProtoWireType::Fixed32, Apply::AbsWatt,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, solarInputW))},
    {EcoFlowDisplayPropertyDecoder::kCmsBattSoc, ProtoWireType::Fixed32, Apply::SocCms, 0},
    {EcoFlowDisplayPropertyDecoder::kBmsBattSoc, ProtoWireType::Fixed32, Apply::SocBms, 0},
    {EcoFlowDisplayPropertyDecoder::kCmsDsgRemTime, ProtoWireType::Varint, Apply::Remain, 0},
    {EcoFlowDisplayPropertyDecoder::kCmsChgRemTime, ProtoWireType::Varint, Apply::RemainIfEmpty, 0},
    {EcoFlowDisplayPropertyDecoder::kFlowInfoAcOut, ProtoWireType::Varint, Apply::FlowOn,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, acOutputOn))},
    {EcoFlowDisplayPropertyDecoder::kFlowInfo12v, ProtoWireType::Varint, Apply::FlowOn,
     static_cast<uint16_t>(offsetof(models::PowerStationTelemetry, dcOutputOn))},
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

}  // namespace

void applyDisplayField(EcoFlowDisplayPropertyDecoder& self, const ProtoFieldView& field) {
  const FieldSpec* spec = findSpec(field.number, field.wireType);
  if (spec == nullptr) {
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
  }
}

void EcoFlowDisplayPropertyDecoder::reset() {
  snapshot_ = models::PowerStationTelemetry{};
  for (size_t i = 0; i < kUsbLaneCount; ++i) {
    usbWatts_[i] = 0.0f;
    haveUsbWatts_[i] = false;
  }
  haveCmsSoc_ = false;
  dirty_ = false;
}

bool EcoFlowDisplayPropertyDecoder::flowIsOn(uint32_t value) {
  return (value & 0x2U) != 0;
}

int16_t EcoFlowDisplayPropertyDecoder::wattsToI16(float wattsAbs) {
  if (wattsAbs > 32000.0f) {
    return 32000;
  }
  return static_cast<int16_t>(wattsAbs + 0.5f);
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
  // Temperature field numbers not verified — leave haveTemperature false (UI: --).
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
