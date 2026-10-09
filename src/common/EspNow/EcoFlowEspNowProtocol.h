#pragma once

#include <cstddef>
#include <cstdint>

/// Wire format for EcoFlow DELTA 3 BLE → Wattcycle ESP-NOW bridge.
/// Single source of truth for hub RX and bridge TX (monorepo common/).
namespace ecoflow_bridge {

#pragma pack(push, 1)
struct EspNowPacketV1 {
  uint32_t magic = 0;   // kMagic
  uint8_t version = 0;  // kVersion
  uint8_t flags = 0;    // ble / valid / ports / encrypted / bridge ESP
  uint8_t socPercent = 0;
  uint8_t reserved = 0;
  int16_t acOutputW = 0;
  int16_t acInputW = 0;
  int16_t dcOutputW = 0;
  int16_t solarInputW = 0;
  int16_t usbOutputW = 0;
  int16_t tempDc = 0;  // °C * 10
  uint16_t remainMinutes = 0;
  uint32_t seq = 0;
  uint32_t telemetryCount = 0;
  char lastError[24] = {};
  // Bridge ESP health (S3)
  uint8_t cpu0Percent = 0;
  uint8_t cpu1Percent = 0;
  int16_t chipTempDc = 0;  // °C * 10
  uint16_t heapKb = 0;
  uint32_t uptimeSec = 0;
};
#pragma pack(pop)

static constexpr uint32_t kMagic = 0x50334645u;  // 'EF3P' LE
static constexpr uint8_t kVersion = 1;
static constexpr size_t kPacketSize = sizeof(EspNowPacketV1);
static constexpr size_t kPmkBytes = 16;

// Encrypted ESP-NOW frames are capped at 147 bytes (CCMP).
static_assert(kPacketSize <= 147, "ESP-NOW encrypted payload must fit in one frame");

enum Flags : uint8_t {
  kFlagBleConnected = 1u << 0,
  kFlagTelemetryValid = 1u << 1,
  kFlagAcOutputOn = 1u << 2,
  kFlagDcOutputOn = 1u << 3,
  kFlagEncrypted = 1u << 4,
  kFlagBridgeEspValid = 1u << 5,
  kFlagUsbOutputOn = 1u << 6,
  kFlagTempValid = 1u << 7,
};

}  // namespace ecoflow_bridge
