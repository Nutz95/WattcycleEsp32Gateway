#pragma once

#include <cstddef>
#include <cstdint>

/// Wire format for XT369P SPP → Wattcycle ESP-NOW bridge.
/// Single source of truth for hub RX and bridge TX (monorepo common/).
namespace xt369p_bridge {

#pragma pack(push, 1)
struct EspNowPacketV1 {
  uint32_t magic = 0;       // kMagic
  uint8_t version = 0;      // kVersion
  uint8_t flags = 0;        // spp / meter / checksum / encrypted
  uint16_t voltageCv = 0;   // volts * 100
  int32_t currentMa = 0;    // amps * 1000
  int32_t powerCw = 0;      // watts * 100
  uint32_t capacityCAh = 0; // Ah * 100
  uint32_t energyCWh = 0;   // Wh * 100
  int16_t tempDc = 0;       // °C * 10
  uint32_t runtimeS = 0;
  uint32_t frameCount = 0;
  uint32_t seq = 0;
  char sppTarget[16] = {};
  char lastError[24] = {};
  // v2: bridge ESP health (TTGO running the SPP client)
  uint8_t cpu0Percent = 0;
  uint8_t cpu1Percent = 0;
  int16_t chipTempDc = 0;   // °C * 10
  uint16_t heapKb = 0;
  uint32_t uptimeSec = 0;
};
#pragma pack(pop)

struct EspNowCommandV1 {
  uint32_t magic = 0;   // kCmdMagic
  uint8_t version = 0;  // kVersion
  uint8_t command = 0;  // MeterCommand wire value (0x01/02/03/05)
  uint16_t reserved = 0;
};

static constexpr uint32_t kMagic = 0x31503658u;     // 'X6P1' LE telemetry
static constexpr uint32_t kCmdMagic = 0x31433658u;  // 'X6C1' LE command
static constexpr uint8_t kVersion = 2;
static constexpr size_t kPacketSize = sizeof(EspNowPacketV1);
static constexpr size_t kCommandSize = sizeof(EspNowCommandV1);
static constexpr size_t kPmkBytes = 16;

// Encrypted ESP-NOW frames are capped at 147 bytes (CCMP).
static_assert(kPacketSize <= 147, "ESP-NOW encrypted payload must fit in one frame");
static_assert(kCommandSize <= 147, "ESP-NOW encrypted command must fit in one frame");

enum Flags : uint8_t {
  kFlagSppConnected = 1u << 0,
  kFlagMeterValid = 1u << 1,
  kFlagChecksumOk = 1u << 2,
  kFlagEncrypted = 1u << 3,
};

}  // namespace xt369p_bridge
