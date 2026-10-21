#include "EspNow/EspNowTelemetryReceiver.h"

#include "Config/TimingConstants.h"
#include "EspNow/EspNowPacketApply.h"
#include "Telemetry/EcoFlowBridgeTelemetry.h"
#include "Telemetry/SolarBridgeTelemetry.h"
#include "Util/MacAddress.h"

#include <cstring>

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace wattcycle::espnow_rx {

EspNowTelemetryReceiver* EspNowTelemetryReceiver::instance_ = nullptr;

EspNowTelemetryReceiver::EspNowTelemetryReceiver(telemetry::ITelemetryStore& store)
    : store_(store) {}

bool EspNowTelemetryReceiver::begin(const char* bmsBridgeMac, const char* xtBridgeMac,
                                    const char* ecoflowBridgeMac, const char* pmk,
                                    uint8_t channel) {
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
}

void EspNowTelemetryReceiver::loop() {
  if (pendingSolar_ || pendingEcoFlow_ || pendingBmsTelemetry_ || pendingBmsProduct_) {
    applyPending();
  }
  using wattcycle::config::TimingConstants;
  const auto solar = store_.solar();
  if (solar.receivedAtMs != 0 &&
      (millis() - solar.receivedAtMs) > TimingConstants::kSolarLinkStaleMs && solar.linkFresh) {
    telemetry::SolarBridgeTelemetry stale = solar;
    stale.linkFresh = false;
    stale.sppConnected = false;
    stale.meterValid = false;
    store_.updateSolar(stale);
  }
  const auto ecoflow = store_.ecoflow();
  if (ecoflow.receivedAtMs != 0 &&
      (millis() - ecoflow.receivedAtMs) > TimingConstants::kEcoFlowLinkStaleMs &&
      ecoflow.linkFresh) {
    telemetry::EcoFlowBridgeTelemetry stale = ecoflow;
    stale.linkFresh = false;
    stale.bleConnected = false;
    stale.meterValid = false;
    store_.updateEcoFlow(stale);
  }
  const auto battery = store_.battery();
  if (battery.updatedAtMs != 0 &&
      (millis() - battery.updatedAtMs) > TimingConstants::kBmsTelemetryStaleMs) {
    const auto status = store_.status();
    if (status.telemetryFresh || status.bleConnected) {
      store_.setBmsLink(false, false);
    }
  }
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
  ingestEspNowFrame(mac, data, len, xtPeerMac_, hasXtPeer_, pendingSolar_, pendingEcoFlow_,
                    pendingBmsTelemetry_, pendingBmsProduct_, solarPacket_, ecoflowPacket_,
                    bmsPacket_, productPacket_);
}

bool EspNowTelemetryReceiver::ensurePeer(const uint8_t mac[6]) {
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
}

bool EspNowTelemetryReceiver::sendMeterCommand(uint8_t command) {
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
