#include "Telemetry/BinaryTelemetryCodec.h"

#include "Bms/Protocol/ProtocolConstants.h"

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

  // v4: bridge ESP health (zeros when bridgeEspValid is false)
  writeU8(cursor, solar.bridgeEsp.cpuCore0Percent);
  writeU8(cursor, solar.bridgeEsp.cpuCore1Percent);
  writeI16(cursor, toDeciCelsius(solar.bridgeEsp.chipTemperatureC));
  writeU16(cursor, static_cast<uint16_t>(solar.bridgeEsp.freeHeapBytes / 1024u));
  writeU32(cursor, solar.bridgeEsp.uptimeSeconds);

  const size_t written = static_cast<size_t>(cursor - buffer);
  if (written > capacity) {
    return 0;
  }
  return written;
}

}  // namespace wattcycle::telemetry
