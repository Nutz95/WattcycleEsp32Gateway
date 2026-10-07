#include "Telemetry/BinaryTelemetryCodec.h"

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
  if (buffer == nullptr || capacity < 64) {
    return 0;
  }

  const auto battery = store.battery();
  const auto product = store.product();
  const auto warnings = store.warnings();
  const auto gateway = store.status();

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
  writeU8(cursor, flags);

  writeU8(cursor, static_cast<uint8_t>(battery.stateOfChargePercent));
  writeU8(cursor, static_cast<uint8_t>(battery.stateOfHealthPercent));
  writeU16(cursor, static_cast<uint16_t>(battery.moduleVoltage * 100.0f + 0.5f));
  writeI16(cursor, static_cast<int16_t>(battery.currentAmps * 10.0f));
  writeI16(cursor, static_cast<int16_t>(battery.powerWatts));
  writeU16(cursor, static_cast<uint16_t>(battery.remainingCapacityAh * 10.0f + 0.5f));
  writeU16(cursor, static_cast<uint16_t>(battery.totalCapacityAh * 10.0f + 0.5f));
  writeU16(cursor, static_cast<uint16_t>(battery.designCapacityAh * 10.0f + 0.5f));
  writeU16(cursor, battery.cycleNumber);
  writeI16(cursor, toDeciCelsius(battery.mosTemperatureC));
  writeI16(cursor, toDeciCelsius(battery.pcbTemperatureC));
  writeU32(cursor, battery.updatedAtMs);

  writeU8(cursor, battery.cellCount);
  for (uint8_t i = 0; i < battery.cellCount; ++i) {
    writeU16(cursor, static_cast<uint16_t>(battery.cellVoltages[i] * 1000.0f + 0.5f));
  }

  const uint8_t cellTempCount =
      battery.temperatureCount >= 2
          ? static_cast<uint8_t>(battery.temperatureCount - 2)
          : static_cast<uint8_t>(0);
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

  const size_t written = static_cast<size_t>(cursor - buffer);
  if (written > capacity) {
    return 0;
  }
  return written;
}

}  // namespace wattcycle::telemetry
