#pragma once

#include <cstdint>

namespace ecoflow::ble {

/// Wire constants for EcoFlow V3 BLE auth + pd335 telemetry (read-only).
/// Values come from community reverse-engineering (see docs/ECOFLOW_BLE_PROTOCOL.md).
namespace AuthWire {
/// Encrypted outer frame type: handshake / command (vs session ciphertext).
constexpr uint8_t kFrameCommand = 0x00;

/// Device address byte used as `src` on replies from the station BMS/PD MCU.
constexpr uint8_t kAddrDevice = 0x35;
/// Bridge address byte used as `src` when we write session packets.
constexpr uint8_t kAddrBridge = 0x21;

/// Product / auth command set (same as device address on DELTA 3 V3).
constexpr uint8_t kCmdSetAuth = 0x35;
/// RTC / time command set.
constexpr uint8_t kCmdSetRtc = 0x01;

/// Device asks host to set wall-clock (empty payload).
constexpr uint8_t kCmdSetRetTime = 0x52;
/// Device asks host to confirm wall-clock.
constexpr uint8_t kCmdCheckRetTime = 0x53;
/// Session: query auth status after KeyInfo.
constexpr uint8_t kCmdAuthStatus = 0x89;
/// Session: MD5(user_id+serial) auto-auth response.
constexpr uint8_t kCmdAutoAuth = 0x86;

/// pd335 DisplayPropertyUpload (SoC / watts / port flags).
constexpr uint8_t kDisplayCmdSet = 0xFE;
constexpr uint8_t kDisplayCmdId = 0x15;

/// Simple-payload type byte for KeyInfo ciphertext.
constexpr uint8_t kSimpleTypeKeyInfo = 0x02;
/// Request KeyInfo after ECDH shared secret is ready.
constexpr uint8_t kSimpleGetKeyInfo = 0x02;

/// Auth result payload[0] == success.
constexpr uint8_t kAuthResultOk = 0x00;

/// Fallback Unix seconds when S3 has no NTP yet (RTC reply must be non-zero).
constexpr uint32_t kRtcFallbackUnix = 1700000000u;
}  // namespace AuthWire

}  // namespace ecoflow::ble
