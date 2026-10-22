#include "Telemetry/BinaryTelemetryCodec.h"

#include "Bms/Protocol/ProtocolConstants.h"
#include "EspNow/EcoFlowEspNowProtocol.h"

#include <cstring>

namespace wattcycle::telemetry {
namespace {

void writeU8(uint8_t*& cursor, uint8_t value) {
  *cursor++ = value;
}

void writeU16(uint8_t*& cursor, uint16_t value) {
  cursor[0] = static_cast<uint8_t>(value & 0xFF);
  cursor[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  cursor += 2;
}

void writeI16(uint8_t*& cursor, int16_t value) {
  writeU16(cursor, static_cast<uint16_t>(value));
}

void writeU32(uint8_t*& cursor, uint32_t value) {
  writeU16(cursor, static_cast<uint16_t>(value & 0xFFFF));
  writeU16(cursor, static_cast<uint16_t>((value >> 16) & 0xFFFF));
}

void writeI32(uint8_t*& cursor, int32_t value) {
  writeU32(cursor, static_cast<uint32_t>(value));
}

void writeFixedString(uint8_t*& cursor, const char* text, size_t fieldLength) {
  const size_t textLength = text == nullptr ? 0 : std::strlen(text);
  const size_t copyLength = textLength < fieldLength ? textLength : fieldLength;
  if (copyLength > 0) {
    std::memcpy(cursor, text, copyLength);
  }
  if (copyLength < fieldLength) {
    std::memset(cursor + copyLength, 0, fieldLength - copyLength);
  }
  cursor += fieldLength;
}

int16_t toDeciCelsius(float celsius) {
  return static_cast<int16_t>(celsius * 10.0f);
}

void writeEcoFlowDetail(uint8_t*& cursor, const EcoFlowBridgeTelemetry& ecoflow) {
  uint8_t extra = 0;
  if (ecoflow.acInputOn) extra |= ecoflow_bridge::kExtraAcInputOn;
  if (ecoflow.pvInputOn) extra |= ecoflow_bridge::kExtraPvInputOn;
  if (ecoflow.haveCellTemp) extra |= ecoflow_bridge::kExtraCellTemp;
  if (ecoflow.haveSoh) extra |= ecoflow_bridge::kExtraSoh;
  if (ecoflow.haveFreq) extra |= ecoflow_bridge::kExtraFreq;
  if (ecoflow.haveErr) extra |= ecoflow_bridge::kExtraErr;
  if (ecoflow.haveChgState) extra |= ecoflow_bridge::kExtraChgState;
  if (ecoflow.haveFan) extra |= ecoflow_bridge::kExtraFan;
  writeU8(cursor, extra);
  writeU8(cursor, ecoflow.sohPercent);
  writeU8(cursor, ecoflow.acInFreqHz);
  writeU8(cursor, ecoflow.chgDsgState);
  writeU8(cursor, ecoflow.fanLevel);
  writeU8(cursor, ecoflow.chargeLimitPercent);
  writeU8(cursor, ecoflow.dischargeLimitPercent);
  writeI16(cursor, ecoflow.inSumW);
  writeI16(cursor, ecoflow.outSumW);
  writeI16(cursor, ecoflow.bmsPowerW);
  writeI16(cursor, ecoflow.minCellTempC);
  writeI16(cursor, ecoflow.maxCellTempC);
  writeI16(cursor, ecoflow.minMosTempC);
  writeI16(cursor, ecoflow.maxMosTempC);
  writeU32(cursor, ecoflow.errCode);
}

void writeEcoFlowRuntime(uint8_t*& cursor, const EcoFlowBridgeTelemetry& ecoflow) {
  uint8_t flags = 0;
  if (ecoflow.haveAcInElec) flags |= ecoflow_bridge::kRuntimeAcIn;
  if (ecoflow.haveAcOutElec) flags |= ecoflow_bridge::kRuntimeAcOut;
  if (ecoflow.haveBmsElec) flags |= ecoflow_bridge::kRuntimeBms;
  if (ecoflow.haveCellVolts) flags |= ecoflow_bridge::kRuntimeCellVolts;
  if (ecoflow.haveCapacity) flags |= ecoflow_bridge::kRuntimeCapacity;
  if (ecoflow.havePcsTemp) flags |= ecoflow_bridge::kRuntimePcsTemp;
  if (ecoflow.haveChargeLimit) flags |= ecoflow_bridge::kRuntimeChgLimit;
  if (ecoflow.haveDischargeLimit) flags |= ecoflow_bridge::kRuntimeDsgLimit;
  writeU8(cursor, flags);
  writeI16(cursor, ecoflow.acInVoltsDeci);
  writeI16(cursor, ecoflow.acOutVoltsDeci);
  writeI16(cursor, ecoflow.acInCentiamps);
  writeI16(cursor, ecoflow.acOutCentiamps);
  writeI16(cursor, ecoflow.bmsVoltsCenti);
  writeI16(cursor, ecoflow.bmsCentiamps);
  writeU16(cursor, ecoflow.minCellMv);
  writeU16(cursor, ecoflow.maxCellMv);
  writeU16(cursor, ecoflow.fullCapMah);
  writeU16(cursor, ecoflow.remainCapMah);
  writeI16(cursor, ecoflow.pcsDcTempC);
  writeI16(cursor, ecoflow.pcsAcTempC);
}

}  // namespace

size_t BinaryTelemetryCodec::encode(const ITelemetryStore& store, uint8_t* buffer,
                                    size_t capacity) {
  if (buffer == nullptr || capacity < 128) {
    return 0;
  }

  const auto battery = store.battery();
  const auto product = store.product();
  const auto warnings = store.warnings();
  const auto gateway = store.status();
  const auto esp = store.espHealth();
  const auto solar = store.solar();

  uint8_t* cursor = buffer;
  writeU32(cursor, kMagic);
  writeU8(cursor, kVersion);

  uint8_t flags = 0;
  if (battery.valid) {
    flags |= 0x01;
  }
  if (gateway.wifiConnected) {
    flags |= 0x02;
  }
  if (gateway.bleConnected) {
    flags |= 0x04;
  }
  if (warnings.hasActiveProtection) {
    flags |= 0x08;
  }
  if (warnings.hasFault) {
    flags |= 0x10;
  }
  if (gateway.telemetryFresh) {
    flags |= 0x20;
  }
  if (solar.linkFresh) {
    flags |= 0x40;
  }
  if (solar.sppConnected) {
    flags |= 0x80;
  }
  writeU8(cursor, flags);

  writeU8(cursor, static_cast<uint8_t>(battery.stateOfChargePercent));
  writeU8(cursor, static_cast<uint8_t>(battery.stateOfHealthPercent));
  writeU16(cursor, static_cast<uint16_t>(battery.moduleVoltage * 100.0f + 0.5f));
  writeI16(cursor, static_cast<int16_t>(battery.currentAmps * 10.0f));
  writeI16(cursor, static_cast<int16_t>(battery.powerWatts));
  writeI16(cursor, static_cast<int16_t>(battery.balanceCurrentAmps * 10.0f));
  writeU16(cursor, static_cast<uint16_t>(battery.remainingCapacityAh * 10.0f + 0.5f));
  writeU16(cursor, static_cast<uint16_t>(battery.totalCapacityAh * 10.0f + 0.5f));
  writeU16(cursor, static_cast<uint16_t>(battery.designCapacityAh * 10.0f + 0.5f));
  writeU16(cursor, battery.cycleNumber);
  writeI16(cursor, toDeciCelsius(battery.mosTemperatureC));
  writeI16(cursor, toDeciCelsius(battery.pcbTemperatureC));
  writeU32(cursor, battery.updatedAtMs);

  const uint8_t cellCount =
      battery.cellCount > bms::kMaxCellCount ? bms::kMaxCellCount : battery.cellCount;
  // Worst-case remaining payload after header/fixed fields (cells + balance + temps + rest).
  const size_t worstCaseTail = static_cast<size_t>(cellCount) * 2u +
                               static_cast<size_t>((cellCount + 7u) / 8u) + 1u +
                               static_cast<size_t>(bms::kMaxCellCount) * 2u + 220u;
  if (static_cast<size_t>(cursor - buffer) + worstCaseTail > capacity) {
    return 0;
  }

  writeU8(cursor, cellCount);
  for (uint8_t i = 0; i < cellCount; ++i) {
    writeU16(cursor, static_cast<uint16_t>(battery.cellVoltages[i] * 1000.0f + 0.5f));
  }

  const uint8_t balanceByteCount = static_cast<uint8_t>((cellCount + 7u) / 8u);
  for (uint8_t byteIndex = 0; byteIndex < balanceByteCount; ++byteIndex) {
    uint8_t bits = 0;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      const uint8_t cellIndex = static_cast<uint8_t>(byteIndex * 8 + bit);
      if (cellIndex < cellCount && warnings.cellBalancing[cellIndex]) {
        bits |= static_cast<uint8_t>(1u << bit);
      }
    }
    writeU8(cursor, bits);
  }

  uint8_t cellTempCount =
      battery.temperatureCount >= 2
          ? static_cast<uint8_t>(battery.temperatureCount - 2)
          : static_cast<uint8_t>(0);
  if (cellTempCount > bms::kMaxCellCount) {
    cellTempCount = bms::kMaxCellCount;
  }
  writeU8(cursor, cellTempCount);
  for (uint8_t i = 0; i < cellTempCount; ++i) {
    writeI16(cursor, toDeciCelsius(battery.cellTemperaturesC[i]));
  }

  writeU8(cursor, warnings.statusRegister1);
  writeU8(cursor, warnings.statusRegister2);
  writeU8(cursor, warnings.statusRegister5);
  writeU8(cursor, warnings.warningRegister1);
  writeU8(cursor, warnings.warningRegister2);

  writeFixedString(cursor, product.firmwareVersion, 20);
  writeFixedString(cursor, product.manufacturerName, 20);
  writeFixedString(cursor, product.serialNumber, 20);
  writeFixedString(cursor, gateway.wifiIp, 16);
  writeFixedString(cursor, gateway.bleAddress, 18);
  writeFixedString(cursor, gateway.lastError, 64);

  writeU8(cursor, esp.cpuCore0Percent);
  writeU8(cursor, esp.cpuCore1Percent);
  writeI16(cursor, toDeciCelsius(esp.chipTemperatureC));
  writeU16(cursor, static_cast<uint16_t>(esp.freeHeapBytes / 1024u));
  writeU32(cursor, esp.uptimeSeconds);

  // XT369P solar bridge (ESP-NOW)
  uint8_t solarFlags = 0;
  if (solar.meterValid) {
    solarFlags |= 0x01;
  }
  if (solar.checksumOk) {
    solarFlags |= 0x02;
  }
  if (solar.espNowEncrypted) {
    solarFlags |= 0x04;
  }
  if (solar.bridgeEspValid) {
    solarFlags |= 0x08;
  }
  writeU8(cursor, solarFlags);
  writeU16(cursor, static_cast<uint16_t>(solar.voltageV * 100.0f + 0.5f));
  writeI32(cursor, static_cast<int32_t>(solar.currentA * 1000.0f));
  writeI32(cursor, static_cast<int32_t>(solar.powerW * 100.0f));
  writeU32(cursor, static_cast<uint32_t>(solar.capacityAh * 1000.0f + 0.5f));
  writeU32(cursor, static_cast<uint32_t>(solar.energyWh * 1000.0f + 0.5f));
  writeI16(cursor, toDeciCelsius(solar.temperatureC));
  writeU32(cursor, solar.runtimeS);
  writeU32(cursor, solar.frameCount);
  writeU32(cursor, solar.seq);
  writeFixedString(cursor, solar.sppTarget, 16);
  writeFixedString(cursor, solar.lastError, 24);

  // XT bridge ESP health (zeros when bridgeEspValid is false)
  writeU8(cursor, solar.bridgeEsp.cpuCore0Percent);
  writeU8(cursor, solar.bridgeEsp.cpuCore1Percent);
  writeI16(cursor, toDeciCelsius(solar.bridgeEsp.chipTemperatureC));
  writeU16(cursor, static_cast<uint16_t>(solar.bridgeEsp.freeHeapBytes / 1024u));
  writeU32(cursor, solar.bridgeEsp.uptimeSeconds);

  // v5 trailer: bit0 = BMS bridge ESP valid, bit1 = NTP synced.
  const auto bmsBridge = store.bmsBridgeHealth();
  const bool bmsBridgeOk = store.bmsBridgeHealthValid();
  uint8_t trailerFlags = 0;
  if (bmsBridgeOk) {
    trailerFlags |= 0x01u;
  }
  if (gateway.ntpSynced) {
    trailerFlags |= 0x02u;
  }
  writeU8(cursor, trailerFlags);
  writeU8(cursor, bmsBridge.cpuCore0Percent);
  writeU8(cursor, bmsBridge.cpuCore1Percent);
  writeI16(cursor, toDeciCelsius(bmsBridge.chipTemperatureC));
  writeU16(cursor, static_cast<uint16_t>(bmsBridge.freeHeapBytes / 1024u));
  writeU32(cursor, bmsBridge.uptimeSeconds);

  // v6: EcoFlow power-station bridge (ESP-NOW)
  const auto ecoflow = store.ecoflow();
  uint8_t ecoFlags = 0;
  if (ecoflow.linkFresh) {
    ecoFlags |= 0x01u;
  }
  if (ecoflow.bleConnected) {
    ecoFlags |= 0x02u;
  }
  if (ecoflow.meterValid) {
    ecoFlags |= 0x04u;
  }
  if (ecoflow.acOutputOn) {
    ecoFlags |= 0x08u;
  }
  if (ecoflow.dcOutputOn) {
    ecoFlags |= 0x10u;
  }
  if (ecoflow.espNowEncrypted) {
    ecoFlags |= 0x20u;
  }
  if (ecoflow.bridgeEspValid) {
    ecoFlags |= 0x40u;
  }
  if (ecoflow.usbOutputOn) {
    ecoFlags |= 0x80u;
  }
  // haveTemperature piggy-backs on reserved byte below (1 = valid).
  writeU8(cursor, ecoFlags);
  writeU8(cursor, ecoflow.socPercent);
  writeU8(cursor, ecoflow.haveTemperature ? 1u : 0u);
  writeI16(cursor, ecoflow.acOutputW);
  writeI16(cursor, ecoflow.acInputW);
  writeI16(cursor, ecoflow.dcOutputW);
  writeI16(cursor, ecoflow.solarInputW);
  writeI16(cursor, ecoflow.usbOutputW);
  writeI16(cursor, toDeciCelsius(ecoflow.temperatureC));
  writeU16(cursor, ecoflow.remainMinutes);
  writeU32(cursor, ecoflow.seq);
  writeU32(cursor, ecoflow.telemetryCount);
  writeFixedString(cursor, ecoflow.lastError, 24);
  writeU8(cursor, ecoflow.bridgeEsp.cpuCore0Percent);
  writeU8(cursor, ecoflow.bridgeEsp.cpuCore1Percent);
  writeI16(cursor, toDeciCelsius(ecoflow.bridgeEsp.chipTemperatureC));
  writeU16(cursor, static_cast<uint16_t>(ecoflow.bridgeEsp.freeHeapBytes / 1024u));
  writeU32(cursor, ecoflow.bridgeEsp.uptimeSeconds);
  // v9: station detail (25 bytes) then runtime. Flag bits are EcoFlowEspNowProtocol.
  writeEcoFlowDetail(cursor, ecoflow);
  writeEcoFlowRuntime(cursor, ecoflow);
  // Optional trailer. Version stays 9 so older pages ignore the extra bytes.
  if (static_cast<size_t>(cursor - buffer) + 3 <= capacity) {
    writeI16(cursor, static_cast<int16_t>(gateway.coprocessorHeapKb));
    writeU8(cursor, gateway.bmsEspNowEncrypted ? 1u : 0u);
  }

  const size_t written = static_cast<size_t>(cursor - buffer);
  if (written > capacity) {
    return 0;
  }
  return written;
}

}  // namespace wattcycle::telemetry
