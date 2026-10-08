#include "EspNow/EspNowTelemetryReceiver.h"

#include "Telemetry/SolarBridgeTelemetry.h"
#include "Util/MacAddress.h"
#include "Util/SafeCopy.h"

#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <esp_now.h>
#endif

namespace wattcycle::espnow_rx {

EspNowTelemetryReceiver* EspNowTelemetryReceiver::instance_ = nullptr;

EspNowTelemetryReceiver::EspNowTelemetryReceiver(telemetry::ITelemetryStore& store)
    : store_(store) {}

bool EspNowTelemetryReceiver::begin(const char* bridgeMac, const char* pmk) {
#ifndef UNIT_TEST
  instance_ = this;
  ready_ = false;

  // Opt-in: classic ESP32 + NimBLE + ESP-NOW coex is fragile. Require a valid
  // XT369P STA MAC before touching esp_now_* at all (avoids boot loops).
  uint8_t bridge[6] = {};
  if (!util::parseMacAddress(bridgeMac, bridge)) {
    Serial.println(F("ESP-NOW skipped (set ESPNOW_BRIDGE_MAC to enable solar RX)"));
    return false;
  }

  encrypt_ = pmk != nullptr && std::strlen(pmk) >= xt369p_bridge::kPmkBytes;
  if (encrypt_) {
    std::memcpy(lmk_, pmk, xt369p_bridge::kPmkBytes);
  } else if (pmk != nullptr && pmk[0] != '\0') {
    Serial.println(F("ESP-NOW: ESPNOW_PMK too short - using plaintext"));
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ESP-NOW RX init failed"));
    return false;
  }

  if (encrypt_) {
    if (esp_now_set_pmk(lmk_) != ESP_OK) {
      Serial.println(F("ESP-NOW set_pmk failed - deinit, solar off"));
      esp_now_deinit();
      return false;
    }
  }

  if (!ensurePeer(bridge)) {
    Serial.println(F("ESP-NOW add bridge peer failed - deinit, solar off"));
    esp_now_deinit();
    return false;
  }
  std::memcpy(peerMac_, bridge, 6);
  hasPeer_ = true;

  esp_now_register_recv_cb(&EspNowTelemetryReceiver::onReceiveTrampoline);
  ready_ = true;
  Serial.printf("ESP-NOW RX listening (enc=%u) heap=%u\n", encrypt_ ? 1u : 0u,
                ESP.getFreeHeap());
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
