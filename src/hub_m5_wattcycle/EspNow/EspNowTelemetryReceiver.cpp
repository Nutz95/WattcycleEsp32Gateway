#include "EspNow/EspNowTelemetryReceiver.h"

#include "EspNow/EspNowPacketApply.h"
#include "Telemetry/EcoFlowBridgeTelemetry.h"
#include "Telemetry/SolarBridgeTelemetry.h"
#include "Util/MacAddress.h"

#include <cstddef>
#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#endif

namespace wattcycle::espnow_rx {

EspNowTelemetryReceiver* EspNowTelemetryReceiver::instance_ = nullptr;

EspNowTelemetryReceiver::EspNowTelemetryReceiver(telemetry::ITelemetryStore& store)
    : store_(store) {}

bool EspNowTelemetryReceiver::begin(const char* bmsBridgeMac, const char* xtBridgeMac,
                                    const char* ecoflowBridgeMac, const char* pmk,
                                    uint8_t channel) {
#ifndef UNIT_TEST
  instance_ = this;
  ready_ = false;

  uint8_t bmsMac[6] = {};
  uint8_t xtMac[6] = {};
  uint8_t ecoMac[6] = {};
  const bool haveBms = util::parseMacAddress(bmsBridgeMac, bmsMac);
  const bool haveXt = util::parseMacAddress(xtBridgeMac, xtMac);
  const bool haveEco = util::parseMacAddress(ecoflowBridgeMac, ecoMac);
  if (!haveBms && !haveXt && !haveEco) {
    Serial.println(
        F("ESP-NOW skipped (set ESPNOW_BMS_BRIDGE_MAC / ESPNOW_BRIDGE_MAC / "
          "ESPNOW_ECOFLOW_BRIDGE_MAC)"));
    return false;
  }

  encrypt_ = pmk != nullptr && std::strlen(pmk) >= xt369p_bridge::kPmkBytes;
  if (encrypt_) {
    std::memcpy(lmk_, pmk, xt369p_bridge::kPmkBytes);
  }

  WiFi.mode(WIFI_STA);
  if (channel >= 1 && channel <= 13) {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect(false, false);
      delay(50);
    }
    if (esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
      Serial.printf("ESP-NOW set_channel(%u) failed\n", channel);
      return false;
    }
    Serial.printf("ESP-NOW RX channel locked to %u (IP optional)\n", channel);
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ESP-NOW RX init failed"));
    return false;
  }
  if (encrypt_ && esp_now_set_pmk(lmk_) != ESP_OK) {
    Serial.println(F("ESP-NOW set_pmk failed"));
    esp_now_deinit();
    return false;
  }

  if (haveBms && !ensurePeer(bmsMac)) {
    Serial.println(F("ESP-NOW add BMS peer failed"));
    esp_now_deinit();
    return false;
  }
  if (haveXt) {
    if (!ensurePeer(xtMac)) {
      Serial.println(F("ESP-NOW add XT peer failed"));
      esp_now_deinit();
      return false;
    }
    std::memcpy(xtPeerMac_, xtMac, 6);
    hasXtPeer_ = true;
  }
  if (haveEco && !ensurePeer(ecoMac)) {
    Serial.println(F("ESP-NOW add EcoFlow peer failed"));
    esp_now_deinit();
    return false;
  }

  esp_now_register_recv_cb(&EspNowTelemetryReceiver::onReceiveTrampoline);
  ready_ = true;
  {
    telemetry::SolarBridgeTelemetry solar = store_.solar();
    solar.espNowEncrypted = encrypt_;
    store_.updateSolar(solar);
  }
  Serial.printf("ESP-NOW multi-peer RX (bms=%u xt=%u eco=%u enc=%u)\n", haveBms ? 1u : 0u,
                haveXt ? 1u : 0u, haveEco ? 1u : 0u, encrypt_ ? 1u : 0u);
  return true;
#else
  (void)bmsBridgeMac;
  (void)xtBridgeMac;
  (void)ecoflowBridgeMac;
  (void)pmk;
  (void)channel;
  ready_ = true;
  return true;
#endif
}

void EspNowTelemetryReceiver::loop() {
  if (pendingSolar_ || pendingEcoFlow_ || pendingBmsTelemetry_ || pendingBmsProduct_) {
    applyPending();
  }
#ifndef UNIT_TEST
  const auto solar = store_.solar();
  if (solar.receivedAtMs != 0 && (millis() - solar.receivedAtMs) > 5000u && solar.linkFresh) {
    telemetry::SolarBridgeTelemetry stale = solar;
    stale.linkFresh = false;
    store_.updateSolar(stale);
  }
  const auto ecoflow = store_.ecoflow();
  if (ecoflow.receivedAtMs != 0 && (millis() - ecoflow.receivedAtMs) > 5000u &&
      ecoflow.linkFresh) {
    telemetry::EcoFlowBridgeTelemetry stale = ecoflow;
    stale.linkFresh = false;
    store_.updateEcoFlow(stale);
  }
  const auto battery = store_.battery();
  if (battery.updatedAtMs != 0 && (millis() - battery.updatedAtMs) > 8000u &&
      store_.status().telemetryFresh) {
    store_.setTelemetryFresh(false);
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
  (void)mac;
  if (data == nullptr || len < 8) {
    return;
  }
  const uint32_t magic = static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
                         (static_cast<uint32_t>(data[2]) << 16) |
                         (static_cast<uint32_t>(data[3]) << 24);
  const uint8_t version = data[4];

  if (magic == xt369p_bridge::kMagic && version > 0 && version <= xt369p_bridge::kVersion) {
    const int requiredLen =
        version >= 2 ? static_cast<int>(sizeof(xt369p_bridge::EspNowPacketV1))
                     : static_cast<int>(offsetof(xt369p_bridge::EspNowPacketV1, cpu0Percent));
    if (len < requiredLen) {
      return;
    }
    xt369p_bridge::EspNowPacketV1 packet = {};
    std::memcpy(&packet, data, static_cast<size_t>(requiredLen));
    solarPacket_ = packet;
    if (mac != nullptr) {
      std::memcpy(xtPeerMac_, mac, 6);
      hasXtPeer_ = true;
    }
    pendingSolar_ = true;
    static uint32_t xtRxCount = 0;
    if ((++xtRxCount % 10u) == 1u) {
      Serial.printf("ESP-NOW RX XT seq=%u n=%u\n", packet.seq, xtRxCount);
    }
    return;
  }

  if (magic == ecoflow_bridge::kMagic && version > 0 && version <= ecoflow_bridge::kVersion) {
    if (len < static_cast<int>(sizeof(ecoflow_bridge::EspNowPacketV1))) {
      return;
    }
    std::memcpy(&ecoflowPacket_, data, sizeof(ecoflowPacket_));
    pendingEcoFlow_ = true;
    static uint32_t ecoRxCount = 0;
    if ((++ecoRxCount % 10u) == 1u) {
      Serial.printf("ESP-NOW RX EF soc=%u n=%u\n", ecoflowPacket_.socPercent, ecoRxCount);
    }
    return;
  }

  if (magic == wattcycle_bridge::kMagic && version > 0 && version <= wattcycle_bridge::kVersion) {
    const int requiredLen =
        version >= 2 ? static_cast<int>(sizeof(wattcycle_bridge::EspNowTelemetryPacketV1))
                     : static_cast<int>(offsetof(wattcycle_bridge::EspNowTelemetryPacketV1,
                                                 cellSensorCount));
    if (len < requiredLen) {
      return;
    }
    bmsPacket_ = {};
    std::memcpy(&bmsPacket_, data, static_cast<size_t>(requiredLen));
    pendingBmsTelemetry_ = true;
    static uint32_t bmsRxCount = 0;
    if ((++bmsRxCount % 10u) == 1u) {
      Serial.printf("ESP-NOW RX BMS soc=%u n=%u\n", bmsPacket_.socPercent, bmsRxCount);
    }
    return;
  }

  if (magic == wattcycle_bridge::kProductMagic && version > 0 &&
      version <= wattcycle_bridge::kProductVersion) {
    if (len < static_cast<int>(sizeof(wattcycle_bridge::EspNowProductPacketV1))) {
      return;
    }
    std::memcpy(&productPacket_, data, sizeof(productPacket_));
    pendingBmsProduct_ = true;
  }
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
  if (!ready_ || !hasXtPeer_) {
    return false;
  }
  if (!ensurePeer(xtPeerMac_)) {
    return false;
  }
  xt369p_bridge::EspNowCommandV1 packet = {};
  packet.magic = xt369p_bridge::kCmdMagic;
  packet.version = xt369p_bridge::kVersion;
  packet.command = command;
  return esp_now_send(xtPeerMac_, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet)) ==
         ESP_OK;
#else
  (void)command;
  return false;
#endif
}

void EspNowTelemetryReceiver::applyPending() {
  if (pendingSolar_) {
    pendingSolar_ = false;
    applySolarPacket(store_, encrypt_, solarPacket_);
  }
  if (pendingEcoFlow_) {
    pendingEcoFlow_ = false;
    applyEcoFlowPacket(store_, encrypt_, ecoflowPacket_);
  }
  if (pendingBmsTelemetry_) {
    pendingBmsTelemetry_ = false;
    applyBmsTelemetryPacket(store_, bmsPacket_);
  }
  if (pendingBmsProduct_) {
    pendingBmsProduct_ = false;
    applyBmsProductPacket(store_, productPacket_);
  }
}

}  // namespace wattcycle::espnow_rx
