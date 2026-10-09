#pragma once

#include "EcoFlow/Crypto/EcoFlowSessionCrypto.h"
#include "EcoFlow/Models/PowerStationTelemetry.h"
#include "EcoFlow/Protocol/EcoFlowDisplayPropertyDecoder.h"
#include "EcoFlow/Protocol/EcoFlowPacketCodec.h"

#include <cstddef>
#include <cstdint>

namespace ecoflow::ble {

/// V3 BLE auth state machine (ECDH → session key → MD5 user/serial). Read-only.
class EcoFlowBleAuth {
 public:
  using WriteFn = bool (*)(void* user, const uint8_t* data, size_t length);

  enum class State : uint8_t {
    Idle = 0,
    WaitPeerPubKey,
    WaitKeyInfo,
    WaitAuthStatus,
    WaitAuthResult,
    Authenticated,
    Failed,
  };

  /// Clear handshake state, buffers, and telemetry snapshot.
  void reset();
  /// Wire GATT write path and EcoFlow account identifiers for MD5 auth.
  void configure(WriteFn writeFn, void* writeUser, const char* serial, const char* userId);
  /// Send local ECDH public key and enter WaitPeerPubKey.
  bool start();
  /// Copy notify bytes only (safe on NimBLE host task — no crypto/write).
  void onNotify(const uint8_t* data, size_t length);
  /// Drain RX + run handshake / decrypt (call from Arduino loop task).
  void poll();

  /// Current V3 auth handshake phase.
  State state() const { return state_; }
  /// True after successful user/serial MD5 auth result.
  bool isAuthenticated() const { return state_ == State::Authenticated; }
  /// Human-readable failure reason when state is Failed.
  const char* lastError() const { return lastError_; }
  /// Latest decoded display-property telemetry fields.
  const models::PowerStationTelemetry& telemetry() const { return telemetry_; }
  /// Count of telemetry snapshots applied since reset.
  uint32_t telemetryCount() const { return telemetryCount_; }

 private:
  bool writeEnc(uint8_t frameType, const uint8_t* payload, size_t payloadLen);
  bool writeSessionPacket(uint8_t src, uint8_t dst, uint8_t cmdSet, uint8_t cmdId,
                          const uint8_t* payload, uint16_t payloadLen);
  bool onSimplePayload(const uint8_t* payload, uint16_t payloadLen);
  bool onSessionCipher(const uint8_t* encPayload, uint16_t encPayloadLen);
  void onPlainPacket(const uint8_t* packetBytes, size_t packetLen);
  bool handleAuthStatus(const protocol::EcoFlowPacket& packet);
  bool handleAuthResult(const protocol::EcoFlowPacket& packet);
  bool handleRtcRequest(const protocol::EcoFlowPacket& packet);
  bool handleDisplayUpload(const protocol::EcoFlowPacket& packet);
  static bool isRtcAck(const protocol::EcoFlowPacket& packet);
  void fail(const char* message);
  void logTelemetrySnapshot();

  WriteFn writeFn_ = nullptr;
  void* writeUser_ = nullptr;
  char serial_[24] = {};
  char userId_[24] = {};
  char lastError_[96] = {};
  State state_ = State::Idle;
  crypto::EcoFlowSessionCrypto crypto_{};
  uint8_t rxBuffer_[768] = {};
  size_t rxLength_ = 0;
  // Large work buffers live on the object (not NimBLE/Arduino stack).
  uint8_t workPlain_[512] = {};
  uint8_t workCipher_[288] = {};
  uint8_t workWire_[320] = {};
  uint32_t telemetryCount_ = 0;
  uint32_t lastTelemetryLogMs_ = 0;
  protocol::EcoFlowDisplayPropertyDecoder displayDecoder_{};
  models::PowerStationTelemetry telemetry_{};
};

}  // namespace ecoflow::ble
