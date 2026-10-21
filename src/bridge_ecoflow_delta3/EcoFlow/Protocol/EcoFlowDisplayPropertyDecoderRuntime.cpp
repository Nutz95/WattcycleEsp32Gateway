#include "EcoFlow/Protocol/EcoFlowDisplayPropertyDecoder.h"

namespace ecoflow::protocol {
namespace {

int16_t scaledI16(float value, float scale) {
  return EcoFlowDisplayPropertyDecoder::clampRoundI16(value * scale);
}

void setScaled(int16_t& member, bool& flag, bool& dirty, float value, float scale) {
  member = scaledI16(value, scale);
  flag = true;
  dirty = true;
}

void setPreferredScaled(int16_t& member, bool& havePreferred, bool& flag, bool& dirty, float value,
                        float scale, bool preferred) {
  if (!preferred && havePreferred) {
    return;
  }
  member = scaledI16(value, scale);
  flag = true;
  if (preferred) {
    havePreferred = true;
  }
  dirty = true;
}

bool inCellMv(uint64_t value) {
  return value >= EcoFlowDisplayPropertyDecoder::kMinCellMv &&
         value <= EcoFlowDisplayPropertyDecoder::kMaxCellMv;
}

}  // namespace

void applyRuntimeFloat(models::PowerStationTelemetry& tele, bool& haveCmsVolts, bool& haveCmsAmps,
                       bool& dirty, const ProtoFieldView& field) {
  switch (field.number) {
    case EcoFlowDisplayPropertyDecoder::kTempPcsDc:
      tele.pcsDcTempC = EcoFlowDisplayPropertyDecoder::clampRoundI16(field.float32);
      tele.havePcsTemp = true;
      dirty = true;
      return;
    case EcoFlowDisplayPropertyDecoder::kTempPcsAc:
      tele.pcsAcTempC = EcoFlowDisplayPropertyDecoder::clampRoundI16(field.float32);
      tele.havePcsTemp = true;
      dirty = true;
      return;
    case EcoFlowDisplayPropertyDecoder::kAcInVolts:
      setScaled(tele.acInVoltsDeci, tele.haveAcInElec, dirty, field.float32,
                static_cast<float>(EcoFlowDisplayPropertyDecoder::kDeci));
      return;
    case EcoFlowDisplayPropertyDecoder::kAcOutVolts:
      setScaled(tele.acOutVoltsDeci, tele.haveAcOutElec, dirty, field.float32,
                static_cast<float>(EcoFlowDisplayPropertyDecoder::kDeci));
      return;
    case EcoFlowDisplayPropertyDecoder::kAcInAmps:
      setScaled(tele.acInCentiamps, tele.haveAcInElec, dirty, field.float32,
                static_cast<float>(EcoFlowDisplayPropertyDecoder::kCenti));
      return;
    case EcoFlowDisplayPropertyDecoder::kAcOutAmps:
      setScaled(tele.acOutCentiamps, tele.haveAcOutElec, dirty, field.float32,
                static_cast<float>(EcoFlowDisplayPropertyDecoder::kCenti));
      return;
    case EcoFlowDisplayPropertyDecoder::kBmsVolts:
      setPreferredScaled(tele.bmsVoltsCenti, haveCmsVolts, tele.haveBmsElec, dirty, field.float32,
                         static_cast<float>(EcoFlowDisplayPropertyDecoder::kCenti), false);
      return;
    case EcoFlowDisplayPropertyDecoder::kCmsVolts:
      setPreferredScaled(tele.bmsVoltsCenti, haveCmsVolts, tele.haveBmsElec, dirty, field.float32,
                         static_cast<float>(EcoFlowDisplayPropertyDecoder::kCenti), true);
      return;
    case EcoFlowDisplayPropertyDecoder::kBmsAmps:
      setPreferredScaled(tele.bmsCentiamps, haveCmsAmps, tele.haveBmsElec, dirty, field.float32,
                         static_cast<float>(EcoFlowDisplayPropertyDecoder::kCenti), false);
      return;
    case EcoFlowDisplayPropertyDecoder::kCmsAmps:
      setPreferredScaled(tele.bmsCentiamps, haveCmsAmps, tele.haveBmsElec, dirty, field.float32,
                         static_cast<float>(EcoFlowDisplayPropertyDecoder::kCenti), true);
      return;
    default:
      return;
  }
}

void applyRuntimeVarint(models::PowerStationTelemetry& tele, bool& dirty, const ProtoFieldView& field) {
  switch (field.number) {
    case EcoFlowDisplayPropertyDecoder::kBmsMinCellMv:
      if (!inCellMv(field.varint)) {
        return;
      }
      tele.minCellMv = static_cast<uint16_t>(field.varint);
      tele.haveCellVolts = true;
      dirty = true;
      return;
    case EcoFlowDisplayPropertyDecoder::kBmsMaxCellMv:
      if (!inCellMv(field.varint)) {
        return;
      }
      tele.maxCellMv = static_cast<uint16_t>(field.varint);
      tele.haveCellVolts = true;
      dirty = true;
      return;
    case EcoFlowDisplayPropertyDecoder::kBmsFullCapMah:
      if (field.varint == 0 || field.varint > EcoFlowDisplayPropertyDecoder::kMaxCapMah) {
        return;
      }
      tele.fullCapMah = static_cast<uint16_t>(field.varint);
      tele.haveCapacity = true;
      dirty = true;
      return;
    case EcoFlowDisplayPropertyDecoder::kBmsRemainCapMah:
      if (field.varint > EcoFlowDisplayPropertyDecoder::kMaxCapMah) {
        return;
      }
      tele.remainCapMah = static_cast<uint16_t>(field.varint);
      tele.haveCapacity = true;
      dirty = true;
      return;
    default:
      return;
  }
}

void EcoFlowDisplayPropertyDecoder::applyRuntime(const ProtoFieldView& field) {
  if (field.wireType == ProtoWireType::Fixed32) {
    applyRuntimeFloat(snapshot_, haveCmsVolts_, haveCmsAmps_, dirty_, field);
    return;
  }
  if (field.wireType == ProtoWireType::Varint) {
    applyRuntimeVarint(snapshot_, dirty_, field);
  }
}

}  // namespace ecoflow::protocol
