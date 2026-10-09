#include "EcoFlow/Ble/EcoFlowBleAuth.h"

#include "EcoFlow/Ble/EcoFlowBleAuthConstants.h"
#include "EcoFlow/Crypto/EcoFlowAuthToken.h"
#include "EcoFlow/Models/PowerStationTelemetry.h"
#include "EcoFlow/Protocol/EcoFlowPacketCodec.h"

#include <Arduino.h>
#include <ctime>

namespace ecoflow::ble {

using ecoflow::protocol::EcoFlowPacket;
using ecoflow::protocol::EcoFlowPacketCodec;
using AuthWire::kAddrBridge;
using AuthWire::kAddrDevice;
using AuthWire::kAuthResultOk;
using AuthWire::kCmdAuthStatus;
using AuthWire::kCmdAutoAuth;
using AuthWire::kCmdCheckRetTime;
using AuthWire::kCmdSetAuth;
using AuthWire::kCmdSetRetTime;
using AuthWire::kCmdSetRtc;
using AuthWire::kDisplayCmdId;
using AuthWire::kDisplayCmdSet;
using AuthWire::kFrameCommand;
using AuthWire::kRtcFallbackUnix;
using AuthWire::kSimpleGetKeyInfo;
using AuthWire::kSimpleTypeKeyInfo;

namespace {
constexpr uint32_t kTelemetryLogIntervalMs = 5000;

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

size_t ecdhKeyBytes(uint8_t curveNum) {
  if (curveNum == 1) {
    return 52;
  }
  if (curveNum == 2) {
    return 56;
  }
  if (curveNum == 3 || curveNum == 4) {
    return 64;
  }
  return 40;
}
}  // namespace

bool EcoFlowBleAuth::onSimplePayload(const uint8_t* payload, uint16_t payloadLen) {
  if (payload == nullptr || payloadLen == 0) {
    fail("empty simple payload");
    return false;
  }
  logHex("rx-simple", payload, payloadLen);

  if (state_ == State::WaitPeerPubKey) {
    if (payloadLen < 3) {
      fail("peer pubkey too short");
      return false;
    }
    const size_t keySize = ecdhKeyBytes(payload[2]);
    const bool secretOk =
        payloadLen >= 3 + keySize && crypto_.computeSharedSecret(payload + 3, keySize);
    if (!secretOk) {
      fail("shared secret failed");
      return false;
    }
    const uint8_t getKeyInfo = kSimpleGetKeyInfo;
    if (!writeEnc(kFrameCommand, &getKeyInfo, 1)) {
      fail("getKeyInfo write failed");
      return false;
    }
    state_ = State::WaitKeyInfo;
    Serial.println("[ecoflow-auth] shared secret OK — requesting KeyInfo");
    return true;
  }

  if (payloadLen < 17 || payload[0] != kSimpleTypeKeyInfo) {
    fail("KeyInfo type mismatch");
    return false;
  }
  const size_t plainLen =
      crypto_.decryptShared(payload + 1, payloadLen - 1, workPlain_, sizeof(workPlain_));
  const bool sessionOk =
      plainLen >= 18 && crypto_.deriveSessionKey(workPlain_ + 16, workPlain_);
  if (!sessionOk) {
    fail("session key derive failed");
    return false;
  }
  Serial.println("[ecoflow-auth] session key ready — getAuthStatus");
  if (!writeSessionPacket(kAddrBridge, kAddrDevice, kCmdSetAuth, kCmdAuthStatus, nullptr, 0)) {
    fail("auth status write failed");
    return false;
  }
  state_ = State::WaitAuthStatus;
  return true;
}

bool EcoFlowBleAuth::onSessionCipher(const uint8_t* encPayload, uint16_t encPayloadLen) {
  const size_t plainLen =
      crypto_.decryptSession(encPayload, encPayloadLen, workPlain_, sizeof(workPlain_));
  if (plainLen == 0) {
    Serial.println("[ecoflow-auth] session decrypt failed");
    return true;
  }
  // Handshake path: keep verbose hex. After auth, throttle in onPlainPacket.
  if (state_ != State::Authenticated) {
    logHex("rx-plain", workPlain_, plainLen);
  }
  onPlainPacket(workPlain_, plainLen);
  return true;
}

void EcoFlowBleAuth::logTelemetrySnapshot() {
  const uint32_t now = millis();
  if (lastTelemetryLogMs_ != 0 && (now - lastTelemetryLogMs_) < kTelemetryLogIntervalMs) {
    return;
  }
  lastTelemetryLogMs_ = now;
  const auto& t = telemetry_;
  Serial.printf(
      "[ecoflow] #%lu SoC=%u%% ACin=%dW ACout=%dW DCout=%dW USB=%dW PV=%dW rem=%umin "
      "AC=%s DC=%s USB=%s temp=%s\n",
      static_cast<unsigned long>(telemetryCount_), static_cast<unsigned>(t.socPercent),
      static_cast<int>(t.acInputW), static_cast<int>(t.acOutputW), static_cast<int>(t.dcOutputW),
      static_cast<int>(t.usbOutputW), static_cast<int>(t.solarInputW),
      static_cast<unsigned>(t.remainMinutes), t.acOutputOn ? "on" : "off",
      t.dcOutputOn ? "on" : "off", t.usbOutputOn ? "on" : "off",
      t.haveTemperature ? "ok" : "--");
}

bool EcoFlowBleAuth::handleAuthStatus(const EcoFlowPacket& packet) {
  const bool match = packet.src == kAddrDevice && packet.cmdSet == kCmdSetAuth &&
                     packet.cmdId == kCmdAuthStatus && state_ == State::WaitAuthStatus;
  if (!match) {
    return false;
  }
  char hex32[crypto::EcoFlowAuthToken::kHexBytes];
  const bool tokenOk = crypto::EcoFlowAuthToken::buildUpperHexMd5(userId_, serial_, hex32);
  const bool writeOk =
      tokenOk && writeSessionPacket(kAddrBridge, kAddrDevice, kCmdSetAuth, kCmdAutoAuth,
                                   reinterpret_cast<const uint8_t*>(hex32),
                                   static_cast<uint16_t>(crypto::EcoFlowAuthToken::kHexBytes));
  if (!writeOk) {
    fail("autoAuth write failed");
    return true;
  }
  state_ = State::WaitAuthResult;
  Serial.println("[ecoflow-auth] auth status RX — sent autoAuthentication");
  return true;
}

bool EcoFlowBleAuth::handleAuthResult(const EcoFlowPacket& packet) {
  const bool match =
      packet.src == kAddrDevice && packet.cmdSet == kCmdSetAuth && packet.cmdId == kCmdAutoAuth;
  if (!match) {
    return false;
  }
  const bool success = packet.payloadLength >= 1 && packet.payload != nullptr &&
                       packet.payload[0] == kAuthResultOk;
  if (success) {
    state_ = State::Authenticated;
    lastError_[0] = '\0';
    telemetryCount_ = 0;
    lastTelemetryLogMs_ = 0;
    displayDecoder_.reset();
    telemetry_ = models::PowerStationTelemetry{};
    Serial.println("[ecoflow-auth] AUTH SUCCESS — parsing DisplayPropertyUpload");
  } else {
    fail("auth rejected by device");
  }
  return true;
}

bool EcoFlowBleAuth::handleRtcRequest(const EcoFlowPacket& packet) {
  const bool match = packet.src == kAddrDevice && packet.cmdSet == kCmdSetRtc &&
                     packet.cmdId == kCmdSetRetTime && packet.payloadLength == 0;
  if (!match) {
    return false;
  }
  time_t now = time(nullptr);
  if (now < static_cast<time_t>(kRtcFallbackUnix)) {
    now = static_cast<time_t>(kRtcFallbackUnix);
  }
  const uint32_t sec = static_cast<uint32_t>(now);
  const uint8_t rtc[6] = {
      static_cast<uint8_t>(sec & 0xFFU),
      static_cast<uint8_t>((sec >> 8) & 0xFFU),
      static_cast<uint8_t>((sec >> 16) & 0xFFU),
      static_cast<uint8_t>((sec >> 24) & 0xFFU),
      0,
      0,
  };
  Serial.println("[ecoflow-auth] device requested RTC — replying");
  writeSessionPacket(kAddrBridge, kAddrDevice, kCmdSetRtc, kCmdSetRetTime, rtc, sizeof(rtc));
  writeSessionPacket(kAddrBridge, kAddrDevice, kCmdSetRtc, kCmdCheckRetTime, rtc, sizeof(rtc));
  return true;
}

bool EcoFlowBleAuth::isRtcAck(const EcoFlowPacket& packet) {
  return packet.src == kAddrDevice && packet.cmdSet == kCmdSetRtc &&
         (packet.cmdId == kCmdSetRetTime || packet.cmdId == kCmdCheckRetTime);
}

bool EcoFlowBleAuth::handleDisplayUpload(const EcoFlowPacket& packet) {
  if (packet.cmdSet != kDisplayCmdSet || packet.cmdId != kDisplayCmdId ||
      packet.payload == nullptr || packet.payloadLength == 0) {
    return false;
  }
  ++telemetryCount_;
  if (displayDecoder_.merge(packet.payload, packet.payloadLength, telemetry_)) {
    telemetry_.updatedAtMs = millis();
    logTelemetrySnapshot();
  }
  return true;
}

void EcoFlowBleAuth::onPlainPacket(const uint8_t* packetBytes, size_t packetLen) {
  EcoFlowPacket packet{};
  if (!EcoFlowPacketCodec::decodePacket(packetBytes, packetLen, packet)) {
    Serial.println("[ecoflow-auth] plain packet parse failed");
    return;
  }

  // DELTA 3 streams version-19 frames with payload XOR'd by seq[0].
  if (packet.payload != nullptr && packet.payloadLength > 0 && packet.seq[0] != 0) {
    auto* mutablePayload = const_cast<uint8_t*>(packet.payload);
    packet.payloadLength = EcoFlowPacketCodec::deobfuscatePayload(
        packet.version, packet.seq[0], mutablePayload, packet.payloadLength);
  }

  if (state_ != State::Authenticated) {
    Serial.printf("[ecoflow-auth] pkt src=%02X dst=%02X set=%02X id=%02X plen=%u\n", packet.src,
                  packet.dst, packet.cmdSet, packet.cmdId,
                  static_cast<unsigned>(packet.payloadLength));
  }

  if (handleAuthStatus(packet) || handleAuthResult(packet) || handleRtcRequest(packet) ||
      isRtcAck(packet) || handleDisplayUpload(packet)) {
    return;
  }
}

}  // namespace ecoflow::ble
