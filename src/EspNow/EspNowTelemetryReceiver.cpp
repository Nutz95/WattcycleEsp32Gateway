#include "EspNow/EspNowTelemetryReceiver.h"

#include "Telemetry/SolarBridgeTelemetry.h"
#include "Util/SafeCopy.h"

#include <cstdio>
#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <esp_now.h>
#endif

namespace wattcycle::espnow_rx {

EspNowTelemetryReceiver* EspNowTelemetryReceiver::instance_ = nullptr;

EspNowTelemetryReceiver::EspNowTelemetryReceiver(telemetry::ITelemetryStore& store)
    : store_(store) {}

bool EspNowTelemetryReceiver::parseMac(const char* text, uint8_t out[6]) const {
  if (text == nullptr || text[0] == '\0' || out == nullptr) {
    return false;
  }
  unsigned int b[6] = {};
  if (std::sscanf(text, "%02x:%02x:%02x:%02x:%02x:%02x", &b[0], &b[1], &b[2], &b[3], &b[4],
                  &b[5]) != 6 &&
      std::sscanf(text, "%02X:%02X:%02X:%02X:%02X:%02X", &b[0], &b[1], &b[2], &b[3], &b[4],
                  &b[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; ++i) {
    out[i] = static_cast<uint8_t>(b[i]);
  }
  return true;
}

bool EspNowTelemetryReceiver::begin(const char* bridgeMac, const char* pmk) {
#ifndef UNIT_TEST
  instance_ = this;
  encrypt_ = pmk != nullptr && std::strlen(pmk) >= xt369p_bridge::kPmkBytes;
  if (encrypt_) {
    std::memcpy(lmk_, pmk, xt369p_bridge::kPmkBytes);
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ESP-NOW RX init failed"));
    ready_ = false;
    return false;
  }

  if (encrypt_) {
    if (esp_now_set_pmk(lmk_) != ESP_OK) {
      Serial.println(F("ESP-NOW set_pmk failed"));
      ready_ = false;
      return false;
    }
    uint8_t mac[6] = {};
    if (!parseMac(bridgeMac, mac)) {
      Serial.println(F("ESP-NOW encrypt needs ESPNOW_BRIDGE_MAC (XT369P STA MAC)"));
      ready_ = false;
      return false;
    }
    if (!ensurePeer(mac)) {
      Serial.println(F("ESP-NOW add encrypted bridge peer failed"));
      ready_ = false;
      return false;
    }
    std::memcpy(peerMac_, mac, 6);
    hasPeer_ = true;
  }

  esp_now_register_recv_cb(&EspNowTelemetryReceiver::onReceiveTrampoline);
  ready_ = true;
  Serial.printf("ESP-NOW RX listening for XT369P bridge (enc=%u)\n", encrypt_ ? 1u : 0u);
  return true;
#else
  (void)bridgeMac;
  (void)pmk;
  ready_ = true;
  return true;
#endif
}

void EspNowTelemetryReceiver::loop() {
  if (pending_) {
    applyPending();
  }

#ifndef UNIT_TEST
  // Mark link stale if no packet for a few seconds.
  const auto solar = store_.solar();
  if (solar.receivedAtMs != 0 && (millis() - solar.receivedAtMs) > 5000u) {
    if (solar.linkFresh) {
      telemetry::SolarBridgeTelemetry stale = solar;
      stale.linkFresh = false;
      store_.updateSolar(stale);
    }
  }
#endif
}

bool EspNowTelemetryReceiver::isReady() const {
  return ready_;
}

void EspNowTelemetryReceiver::onReceiveTrampoline(const uint8_t* mac, const uint8_t* data,
                                                  int len) {
  if (instance_ != nullptr) {
    instance_->onReceive(mac, data, len);
  }
}

void EspNowTelemetryReceiver::onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if (data == nullptr || len < static_cast<int>(sizeof(xt369p_bridge::EspNowPacketV1))) {
    return;
  }
  xt369p_bridge::EspNowPacketV1 packet = {};
  std::memcpy(&packet, data, sizeof(packet));
  if (packet.magic != xt369p_bridge::kMagic || packet.version != xt369p_bridge::kVersion) {
    return;
  }
  pendingPacket_ = packet;
  if (mac != nullptr) {
    std::memcpy(pendingMac_, mac, 6);
    std::memcpy(peerMac_, mac, 6);
    hasPeer_ = true;
  }
  pending_ = true;
}

bool EspNowTelemetryReceiver::ensurePeer(const uint8_t mac[6]) {
#ifndef UNIT_TEST
  if (esp_now_is_peer_exist(mac)) {
    return true;
  }
  esp_now_peer_info_t peer = {};
  std::memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = encrypt_;
  if (encrypt_) {
    std::memcpy(peer.lmk, lmk_, xt369p_bridge::kPmkBytes);
  }
  return esp_now_add_peer(&peer) == ESP_OK;
#else
  (void)mac;
  return true;
#endif
}

bool EspNowTelemetryReceiver::sendMeterCommand(uint8_t command) {
#ifndef UNIT_TEST
  if (!ready_ || !hasPeer_) {
    return false;
  }
  if (!ensurePeer(peerMac_)) {
    return false;
  }
  xt369p_bridge::EspNowCommandV1 packet = {};
  packet.magic = xt369p_bridge::kCmdMagic;
  packet.version = xt369p_bridge::kVersion;
  packet.command = command;
  return esp_now_send(peerMac_, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet)) ==
         ESP_OK;
#else
  (void)command;
  return false;
#endif
}

void EspNowTelemetryReceiver::applyPending() {
  if (!pending_) {
    return;
  }
  pending_ = false;
  const xt369p_bridge::EspNowPacketV1 packet = pendingPacket_;

  telemetry::SolarBridgeTelemetry solar = {};
  solar.linkFresh = true;
  solar.sppConnected = (packet.flags & xt369p_bridge::kFlagSppConnected) != 0;
  solar.meterValid = (packet.flags & xt369p_bridge::kFlagMeterValid) != 0;
  solar.checksumOk = (packet.flags & xt369p_bridge::kFlagChecksumOk) != 0;
  solar.voltageV = packet.voltageCv / 100.0f;
  solar.currentA = packet.currentMa / 1000.0f;
  solar.powerW = packet.powerCw / 100.0f;
  solar.capacityAh = packet.capacityCAh / 100.0f;
  solar.energyWh = packet.energyCWh / 100.0f;
  solar.temperatureC = packet.tempDc / 10.0f;
  solar.runtimeS = packet.runtimeS;
  solar.frameCount = packet.frameCount;
  solar.seq = packet.seq;
#ifndef UNIT_TEST
  solar.receivedAtMs = millis();
#endif
  util::copyCString(solar.sppTarget, sizeof(solar.sppTarget), packet.sppTarget);
  util::copyCString(solar.lastError, sizeof(solar.lastError), packet.lastError);
  store_.updateSolar(solar);
}

}  // namespace wattcycle::espnow_rx
