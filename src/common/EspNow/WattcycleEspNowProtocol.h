#pragma once

#include <cstddef>
#include <cstdint>

/// Wire format for Wattcycle BMS BLE → hub ESP-NOW bridge.
/// Fits encrypted ESP-NOW (≤147 B). Primary frame: analog + ≤16 cells + warnings + bridge health.
/// Product strings use a separate infrequent frame (see EspNowProductPacketV1).
namespace wattcycle_bridge {

#pragma pack(push, 1)
struct EspNowTelemetryPacketV1 {
  uint32_t magic = 0;  // kMagic
  uint8_t version = 0;
  uint8_t flags = 0;
  uint32_t seq = 0;
  uint8_t socPercent = 0;
  uint8_t sohPercent = 0;
  uint16_t moduleVoltageCv = 0;  // V * 100
  int16_t currentDa = 0;         // A * 10
  int16_t powerW = 0;
  int16_t balanceCurrentDa = 0;  // A * 10
  uint16_t remainingCAh = 0;     // Ah * 100
  uint16_t totalCAh = 0;         // Ah * 100
  uint16_t designCAh = 0;        // Ah * 100
  uint16_t cycleNumber = 0;
  int16_t mosTempDc = 0;  // °C * 10
  int16_t pcbTempDc = 0;  // °C * 10
  uint8_t cellCount = 0;  // 0..kMaxCells
  uint16_t cellMv[16] = {};
  uint8_t balanceBits[2] = {};  // bit i = cell i balancing
  uint8_t statusRegister1 = 0;
  uint8_t statusRegister2 = 0;
  uint8_t statusRegister5 = 0;
  uint8_t warningRegister1 = 0;
  uint8_t warningRegister2 = 0;
  // Bridge ESP health (TTGO running the BLE client)
  uint8_t cpu0Percent = 0;
  uint8_t cpu1Percent = 0;
  int16_t chipTempDc = 0;
  uint16_t heapKb = 0;
  uint32_t uptimeSec = 0;
  // v2+: up to 4 pack cell-sensor temps (°C * 10). v1 peers leave these zero.
  uint8_t cellSensorCount = 0;
  int16_t cellTempDc[4] = {};
};

struct EspNowProductPacketV1 {
  uint32_t magic = 0;  // kProductMagic
  uint8_t version = 0;
  uint8_t flags = 0;
  uint32_t seq = 0;
  char firmwareVersion[20] = {};
  char manufacturerName[20] = {};
  char serialNumber[20] = {};
  char bleAddress[18] = {};
  char lastError[24] = {};
};
#pragma pack(pop)

static constexpr uint32_t kMagic = 0x31504357u;         // 'WCP1' LE
static constexpr uint32_t kProductMagic = 0x31505257u;  // 'WRP1' LE
static constexpr uint8_t kVersion = 2;
/// Product frame layout is independent of telemetry v2 cell-sensor fields.
static constexpr uint8_t kProductVersion = 1;
static constexpr size_t kMaxCells = 16;
static constexpr size_t kTelemetryPacketSize = sizeof(EspNowTelemetryPacketV1);
static constexpr size_t kProductPacketSize = sizeof(EspNowProductPacketV1);
static constexpr size_t kPmkBytes = 16;

static_assert(kTelemetryPacketSize <= 147, "ESP-NOW encrypted BMS telemetry must fit");
static_assert(kProductPacketSize <= 147, "ESP-NOW encrypted BMS product must fit");
static_assert(kMaxCells == 16, "cell array size must match kMaxCells");

enum Flags : uint8_t {
  kFlagBleConnected = 1u << 0,
  kFlagBatteryValid = 1u << 1,
  kFlagWarningsValid = 1u << 2,
  kFlagEncrypted = 1u << 3,
  kFlagHasProtection = 1u << 4,
  kFlagHasFault = 1u << 5,
  kFlagBridgeEspValid = 1u << 6,
};

}  // namespace wattcycle_bridge
