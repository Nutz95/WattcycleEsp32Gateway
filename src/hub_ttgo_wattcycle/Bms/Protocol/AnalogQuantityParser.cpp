#include "Bms/Protocol/AnalogQuantityParser.h"

#include "Bms/Protocol/ProtocolScaling.h"

namespace wattcycle::bms {
namespace {

uint16_t readUint16Be(const uint8_t* data, size_t offset) {
  return static_cast<uint16_t>((data[offset] << 8) | data[offset + 1]);
}

bool hasEnough(size_t length, size_t offset, size_t needed) {
  return offset + needed <= length;
}

float kelvinTenthsToCelsius(uint16_t rawKelvinTenths) {
  return (static_cast<float>(rawKelvinTenths) -
          static_cast<float>(ProtocolScaling::kTemperatureKelvinOffset)) /
         ProtocolScaling::kTemperatureDivisor;
}

}  // namespace

float AnalogQuantityParser::parseSignedCurrent(uint8_t highByte, uint8_t lowByte) {
  const bool isNegative = (highByte & ProtocolScaling::kCurrentSignMask) != 0;
  const bool hasDecimal = (highByte & ProtocolScaling::kCurrentDecimalMask) != 0;
  const uint16_t raw = static_cast<uint16_t>(
      lowByte |
      ((static_cast<uint16_t>(highByte & ProtocolScaling::kCurrentMagnitudeMask) << 8)));
  float current = hasDecimal ? (raw / ProtocolScaling::kCurrentDecimalDivisor)
                             : static_cast<float>(raw);
  return isNegative ? -current : current;
}

bool AnalogQuantityParser::parse(const uint8_t* data, size_t length, BatteryTelemetry& out) {
  if (data == nullptr || length < 2) {
    return false;
  }

  BatteryTelemetry telemetry;
  size_t offset = 0;

  telemetry.cellCount = data[offset++];
  if (telemetry.cellCount == 0 || telemetry.cellCount > kMaxCellCount) {
    return false;
  }
  if (!hasEnough(length, offset, static_cast<size_t>(telemetry.cellCount) * 2)) {
    return false;
  }
  for (uint8_t i = 0; i < telemetry.cellCount; ++i) {
    telemetry.cellVoltages[i] =
        readUint16Be(data, offset) / ProtocolScaling::kCellVoltageDivisor;
    offset += 2;
  }

  if (!hasEnough(length, offset, 1)) {
    return false;
  }
  telemetry.temperatureCount = data[offset++];
  if (telemetry.temperatureCount < ProtocolScaling::kMinTemperatureCount) {
    return false;
  }

  const size_t tempPayloadBytes =
      static_cast<size_t>(telemetry.temperatureCount) * 2 + 2 /* current */;
  if (!hasEnough(length, offset, tempPayloadBytes)) {
    return false;
  }

  telemetry.mosTemperatureC = kelvinTenthsToCelsius(readUint16Be(data, offset));
  offset += 2;
  telemetry.pcbTemperatureC = kelvinTenthsToCelsius(readUint16Be(data, offset));
  offset += 2;

  const uint8_t cellTempCount = static_cast<uint8_t>(
      telemetry.temperatureCount - ProtocolScaling::kMinTemperatureCount);
  for (uint8_t i = 0; i < cellTempCount && i < kMaxCellCount; ++i) {
    telemetry.cellTemperaturesC[i] = kelvinTenthsToCelsius(readUint16Be(data, offset));
    offset += 2;
  }

  telemetry.currentAmps = parseSignedCurrent(data[offset], data[offset + 1]);
  offset += 2;

  if (!hasEnough(length, offset, ProtocolScaling::kFixedTailFieldBytes)) {
    return false;
  }
  telemetry.moduleVoltage =
      readUint16Be(data, offset) / ProtocolScaling::kModuleVoltageDivisor;
  offset += 2;
  telemetry.remainingCapacityAh =
      readUint16Be(data, offset) / ProtocolScaling::kCapacityDivisor;
  offset += 2;
  telemetry.totalCapacityAh =
      readUint16Be(data, offset) / ProtocolScaling::kCapacityDivisor;
  offset += 2;
  telemetry.cycleNumber = readUint16Be(data, offset);
  offset += 2;
  telemetry.designCapacityAh =
      readUint16Be(data, offset) / ProtocolScaling::kCapacityDivisor;
  offset += 2;
  telemetry.stateOfChargePercent = readUint16Be(data, offset);
  offset += 2;

  // New-protocol tail: SOH + cumulative + remainingTime + reserved + balanceCurrent.
  if (hasEnough(length, offset, 18)) {
    telemetry.stateOfHealthPercent = readUint16Be(data, offset);
    offset += 2;
    offset += 4;  // cumulative capacity
    offset += 4;  // remaining time
    offset += 6;  // reserved
    telemetry.balanceCurrentAmps = parseSignedCurrent(data[offset], data[offset + 1]);
  } else if (hasEnough(length, offset, ProtocolScaling::kStateOfHealthFieldBytes)) {
    telemetry.stateOfHealthPercent = readUint16Be(data, offset);
  }

  telemetry.powerWatts = telemetry.moduleVoltage * telemetry.currentAmps;
  telemetry.valid = true;
  out = telemetry;
  return true;
}

}  // namespace wattcycle::bms
