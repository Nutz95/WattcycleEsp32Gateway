#include "EspNow/EspNowTelemetryPublisher.h"

#include "EspNow/EcoFlowEspNowProtocol.h"
#include "Util/MacAddress.h"
#include "Util/SafeCopy.h"

#include <Arduino.h>
#include <WiFi.h>
#include <cstring>
#include <esp_now.h>
#include <esp_wifi.h>

namespace ecoflow::espnow_tx {

uint8_t EspNowTelemetryPublisher::resolveChannel(const char* wifiSsid,
                                                 uint8_t fixedChannel) const {
  if (fixedChannel >= 1 && fixedChannel <= 13) {
    return fixedChannel;
  }
  if (wifiSsid == nullptr || wifiSsid[0] == '\0') {
    return 1;
  }
  const int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
  uint8_t channel = 1;
  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i) == wifiSsid) {
      channel = static_cast<uint8_t>(WiFi.channel(i));
      break;
    }
  }
  WiFi.scanDelete();
  return channel;
}

bool EspNowTelemetryPublisher::initRadio(const char* wifiSsid, uint8_t fixedChannel) {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  WiFi.setSleep(WIFI_PS_MIN_MODEM);
  Serial.printf("This board STA MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println(F("  -> set as ESPNOW_ECOFLOW_BRIDGE_MAC on the M5 hub"));

  channel_ = resolveChannel(wifiSsid, fixedChannel);
  if (esp_wifi_set_channel(channel_, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    util::copyCString(lastError_, sizeof(lastError_), "set_channel_failed");
    return false;
  }
  if (esp_now_init() != ESP_OK) {
    util::copyCString(lastError_, sizeof(lastError_), "esp_now_init_failed");
    return false;
  }
  return true;
}

bool EspNowTelemetryPublisher::addPeer(const char* pmk) {
  if (encrypt_) {
    uint8_t key[ecoflow_bridge::kPmkBytes] = {};
    std::memcpy(key, pmk, ecoflow_bridge::kPmkBytes);
    if (esp_now_set_pmk(key) != ESP_OK) {
      util::copyCString(lastError_, sizeof(lastError_), "esp_now_set_pmk_failed");
      return false;
    }
  }

  esp_now_peer_info_t peer = {};
  std::memcpy(peer.peer_addr, peerMac_, 6);
  peer.channel = channel_;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = encrypt_;
  if (encrypt_) {
    std::memcpy(peer.lmk, pmk, ecoflow_bridge::kPmkBytes);
  }
  if (esp_now_is_peer_exist(peerMac_)) {
    esp_now_del_peer(peerMac_);
  }
  if (esp_now_add_peer(&peer) != ESP_OK) {
    util::copyCString(lastError_, sizeof(lastError_), "esp_now_add_peer_failed");
    return false;
  }
  return true;
}

bool EspNowTelemetryPublisher::begin(const char* wifiSsid, const char* peerMac,
                                     uint8_t fixedChannel, const char* pmk) {
  ready_ = false;
  channel_ = 0;
  lastError_[0] = '\0';

  if (!util::parseMacAddress(peerMac, peerMac_)) {
    util::copyCString(lastError_, sizeof(lastError_), "bad_peer_mac");
    return false;
  }

  encrypt_ = pmk != nullptr && std::strlen(pmk) >= ecoflow_bridge::kPmkBytes;
  if (!initRadio(wifiSsid, fixedChannel) || !addPeer(pmk)) {
    return false;
  }

  ready_ = true;
  Serial.printf("ESP-NOW TX peer %02X:%02X:%02X:%02X:%02X:%02X ch=%u enc=%u heap=%u\n",
                peerMac_[0], peerMac_[1], peerMac_[2], peerMac_[3], peerMac_[4], peerMac_[5],
                channel_, encrypt_ ? 1u : 0u, ESP.getFreeHeap());
  return true;
}

bool EspNowTelemetryPublisher::isReady() const {
  return ready_;
}

uint8_t EspNowTelemetryPublisher::channel() const {
  return channel_;
}

void EspNowTelemetryPublisher::publish(const models::PowerStationTelemetry& telemetry,
                                       bool bleConnected, uint32_t telemetryCount,
                                       const char* lastError,
                                       const esp_sys::EspHealthSnapshot& health) {
  if (!ready_) {
    return;
  }

  ecoflow_bridge::EspNowPacketV1 packet = {};
  packet.magic = ecoflow_bridge::kMagic;
  packet.version = ecoflow_bridge::kVersion;
  if (bleConnected) {
    packet.flags |= ecoflow_bridge::kFlagBleConnected;
  }
  if (telemetry.valid) {
    packet.flags |= ecoflow_bridge::kFlagTelemetryValid;
  }
  if (telemetry.acOutputOn) {
    packet.flags |= ecoflow_bridge::kFlagAcOutputOn;
  }
  if (telemetry.dcOutputOn) {
    packet.flags |= ecoflow_bridge::kFlagDcOutputOn;
  }
  if (telemetry.usbOutputOn) {
    packet.flags |= ecoflow_bridge::kFlagUsbOutputOn;
  }
  if (telemetry.haveTemperature) {
    packet.flags |= ecoflow_bridge::kFlagTempValid;
  }
  if (encrypt_) {
    packet.flags |= ecoflow_bridge::kFlagEncrypted;
  }
  packet.flags |= ecoflow_bridge::kFlagBridgeEspValid;
  packet.socPercent = telemetry.socPercent;
  packet.acOutputW = telemetry.acOutputW;
  packet.acInputW = telemetry.acInputW;
  packet.dcOutputW = telemetry.dcOutputW;
  packet.solarInputW = telemetry.solarInputW;
  packet.usbOutputW = telemetry.usbOutputW;
  packet.tempDc = telemetry.haveTemperature ? static_cast<int16_t>(telemetry.tempC * 10) : 0;
  packet.remainMinutes = telemetry.remainMinutes;
  packet.seq = ++seq_;
  packet.telemetryCount = telemetryCount;
  util::copyCString(packet.lastError, sizeof(packet.lastError), lastError);

  packet.cpu0Percent = health.cpuCore0Percent;
  packet.cpu1Percent = health.cpuCore1Percent;
  packet.chipTempDc = static_cast<int16_t>(health.chipTemperatureC * 10.0f);
  packet.heapKb = static_cast<uint16_t>(health.freeHeapBytes / 1024u);
  packet.uptimeSec = health.uptimeSeconds;

  const esp_err_t err =
      esp_now_send(peerMac_, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
  if (err != ESP_OK && (seq_ % 20u) == 1u) {
    Serial.printf("ESP-NOW send failed err=%d\n", static_cast<int>(err));
  }
}

const char* EspNowTelemetryPublisher::lastError() const {
  return lastError_;
}

}  // namespace ecoflow::espnow_tx
