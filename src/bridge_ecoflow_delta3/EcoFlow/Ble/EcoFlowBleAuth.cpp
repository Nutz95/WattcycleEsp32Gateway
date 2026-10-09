#include "EcoFlow/Ble/EcoFlowBleAuth.h"

#include "EcoFlow/Models/PowerStationTelemetry.h"
#include "EcoFlow/Protocol/EcoFlowPacketCodec.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>

namespace ecoflow::ble {

using ecoflow::protocol::EcoFlowEncFrame;
using ecoflow::protocol::EcoFlowPacket;
using ecoflow::protocol::EcoFlowPacketCodec;

namespace {
constexpr uint8_t kFrameCommand = 0x00;
constexpr uint8_t kFrameProtocol = 0x01;

void logHex(const char* label, const uint8_t* data, size_t length) {
  const size_t n = length < 96 ? length : 96;
  Serial.printf("[ecoflow-auth] %s len=%u hex=", label, static_cast<unsigned>(length));
  for (size_t i = 0; i < n; ++i) {
    Serial.printf("%02X", data[i]);
  }
  if (length > n) {
    Serial.print("…");
  }
  Serial.println();
}
}  // namespace

void EcoFlowBleAuth::reset() {
  state_ = State::Idle;
  rxLength_ = 0;
  lastError_[0] = '\0';
  telemetryCount_ = 0;
  lastTelemetryLogMs_ = 0;
  displayDecoder_.reset();
  telemetry_ = models::PowerStationTelemetry{};
  crypto_.reset();
}

void EcoFlowBleAuth::configure(WriteFn writeFn, void* writeUser, const char* serial,
                               const char* userId) {
  writeFn_ = writeFn;
  writeUser_ = writeUser;
  std::snprintf(serial_, sizeof(serial_), "%s", serial != nullptr ? serial : "");
  std::snprintf(userId_, sizeof(userId_), "%s", userId != nullptr ? userId : "");
  reset();
}

bool EcoFlowBleAuth::start() {
  if (writeFn_ == nullptr || serial_[0] == '\0' || userId_[0] == '\0') {
    fail("auth config incomplete");
    return false;
  }
  crypto_.reset();
  rxLength_ = 0;
  if (!crypto_.generateKeyPair()) {
    fail("ecdh keygen failed");
    return false;
  }

  uint8_t payload[2 + crypto::EcoFlowSessionCrypto::kPublicKeyBytes];
  payload[0] = 0x01;
  payload[1] = 0x00;
  std::memcpy(payload + 2, crypto_.publicKey(), crypto::EcoFlowSessionCrypto::kPublicKeyBytes);
  if (!writeEnc(kFrameCommand, payload, sizeof(payload))) {
    fail("pubkey write failed");
    return false;
  }
  state_ = State::WaitPeerPubKey;
  Serial.println("[ecoflow-auth] sent ECDH pubkey — waiting peer key");
  return true;
}

void EcoFlowBleAuth::onNotify(const uint8_t* data, size_t length) {
  // NimBLE host task: copy only — poll() does crypto/writes on the loop task.
  if (data == nullptr || length == 0 || state_ == State::Failed) {
    return;
  }
  if (rxLength_ + length > sizeof(rxBuffer_)) {
    fail("rx buffer overflow");
    return;
  }
  std::memcpy(rxBuffer_ + rxLength_, data, length);
  rxLength_ += length;
}

void EcoFlowBleAuth::poll() {
  while (rxLength_ >= EcoFlowPacketCodec::kMinEncBytes) {
    EcoFlowEncFrame frame{};
    const size_t consumed = EcoFlowPacketCodec::decodeEncFrame(rxBuffer_, rxLength_, frame);
    if (consumed == 0) {
      const bool badPrefix = rxBuffer_[0] != EcoFlowPacketCodec::kEncPrefix0 ||
                             (rxLength_ >= 2 && rxBuffer_[1] != EcoFlowPacketCodec::kEncPrefix1);
      if (badPrefix) {
        std::memmove(rxBuffer_, rxBuffer_ + 1, rxLength_ - 1);
        --rxLength_;
        continue;
      }
      break;
    }

    bool ok = true;
    if (state_ == State::WaitPeerPubKey || state_ == State::WaitKeyInfo) {
      ok = onSimplePayload(frame.payload, frame.payloadLength);
    } else if (crypto_.hasSessionKey()) {
      ok = onSessionCipher(frame.payload, frame.payloadLength);
    } else {
      logHex("unexpected-enc", frame.payload, frame.payloadLength);
    }
    std::memmove(rxBuffer_, rxBuffer_ + consumed, rxLength_ - consumed);
    rxLength_ -= consumed;
    if (!ok || state_ == State::Failed) {
      return;
    }
  }
}

bool EcoFlowBleAuth::writeEnc(uint8_t frameType, const uint8_t* payload, size_t payloadLen) {
  if (writeFn_ == nullptr) {
    return false;
  }
  EcoFlowEncFrame frame{};
  frame.frameType = frameType;
  frame.payload = payload;
  frame.payloadLength = static_cast<uint16_t>(payloadLen);
  const size_t n = EcoFlowPacketCodec::encodeEncFrame(frame, workWire_, sizeof(workWire_));
  if (n == 0) {
    return false;
  }
  logHex(frameType == kFrameCommand ? "tx-cmd" : "tx-sess", workWire_, n);
  const bool wrote = writeFn_(writeUser_, workWire_, n);
  return wrote;
}

bool EcoFlowBleAuth::writeSessionPacket(uint8_t src, uint8_t dst, uint8_t cmdSet, uint8_t cmdId,
                                        const uint8_t* payload, uint16_t payloadLen) {
  EcoFlowPacket packet{};
  packet.src = src;
  packet.dst = dst;
  packet.cmdSet = cmdSet;
  packet.cmdId = cmdId;
  packet.dsrc = 1;
  packet.ddst = 1;
  packet.version = 3;
  packet.payload = payload;
  packet.payloadLength = payloadLen;

  const size_t plainLen =
      EcoFlowPacketCodec::encodePacket(packet, workPlain_, sizeof(workPlain_));
  if (plainLen == 0) {
    return false;
  }
  const size_t cipherLen =
      crypto_.encryptSession(workPlain_, plainLen, workCipher_, sizeof(workCipher_));
  if (cipherLen == 0) {
    return false;
  }
  return writeEnc(kFrameProtocol, workCipher_, cipherLen);
}

void EcoFlowBleAuth::fail(const char* message) {
  if (message == nullptr) {
    lastError_[0] = '\0';
  } else {
    std::snprintf(lastError_, sizeof(lastError_), "%s", message);
  }
  state_ = State::Failed;
  Serial.printf("[ecoflow-auth] FAIL: %s\n", lastError_);
}

}  // namespace ecoflow::ble
