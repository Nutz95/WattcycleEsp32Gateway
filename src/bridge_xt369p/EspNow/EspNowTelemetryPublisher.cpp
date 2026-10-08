#include "EspNow/EspNowTelemetryPublisher.h"

#include "EspNow/Xt369pEspNowProtocol.h"
#include "Meter/Protocol/AtorchCommands.h"
#include "Util/MacAddress.h"
#include "Util/SafeCopy.h"

#include <cstdio>
#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#endif

namespace xt369p::espnow_tx {

EspNowTelemetryPublisher* EspNowTelemetryPublisher::instance_ = nullptr;

uint8_t EspNowTelemetryPublisher::resolveChannel(const char* wifiSsid,
                                                 uint8_t fixedChannel) const {
#ifndef UNIT_TEST
  if (fixedChannel >= 1 && fixedChannel <= 13) {
    return fixedChannel;
  }
  if (wifiSsid == nullptr || wifiSsid[0] == '\0') {
    return 1;
  }
  // Passive scan only — no WiFi.begin / no association with the router.
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
#else
  (void)wifiSsid;
  return fixedChannel >= 1 ? fixedChannel : 1;
#endif
}

bool EspNowTelemetryPublisher::begin(const char* wifiSsid, const char* peerMac,
                                     telemetry::ITelemetryStore& store, uint8_t fixedChannel,
                                     const char* pmk) {
#ifndef UNIT_TEST
  ready_ = false;
  channel_ = 0;
  lastError_[0] = '\0';
  store_ = &store;
  instance_ = this;

  if (!util::parseMacAddress(peerMac, peerMac_)) {
    util::copyCString(lastError_, sizeof(lastError_), "bad_peer_mac");
    return false;
  }

  encrypt_ = pmk != nullptr && std::strlen(pmk) >= xt369p_bridge::kPmkBytes;

  // ESP-NOW rides on the Wi-Fi radio driver (STA mode) but we do NOT join the AP
  // and we do NOT run a web stack. Classic BT comes up after this.
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  WiFi.setSleep(WIFI_PS_MIN_MODEM);
  Serial.printf("This board STA MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println(F("  -> set as ESPNOW_BRIDGE_MAC on the Wattcycle hub"));

  channel_ = resolveChannel(wifiSsid, fixedChannel);
  if (esp_wifi_set_channel(channel_, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    util::copyCString(lastError_, sizeof(lastError_), "set_channel_failed");
    return false;
  }

  if (esp_now_init() != ESP_OK) {
    util::copyCString(lastError_, sizeof(lastError_), "esp_now_init_failed");
    return false;
  }

  if (encrypt_) {
    uint8_t key[xt369p_bridge::kPmkBytes] = {};
    std::memcpy(key, pmk, xt369p_bridge::kPmkBytes);
    if (esp_now_set_pmk(key) != ESP_OK) {
      util::copyCString(lastError_, sizeof(lastError_), "esp_now_set_pmk_failed");
      return false;
    }
  }

  esp_now_register_recv_cb(&EspNowTelemetryPublisher::onReceiveTrampoline);

  esp_now_peer_info_t peer = {};
  std::memcpy(peer.peer_addr, peerMac_, 6);
  peer.channel = channel_;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = encrypt_;
  if (encrypt_) {
    std::memcpy(peer.lmk, pmk, xt369p_bridge::kPmkBytes);
  }
  if (esp_now_is_peer_exist(peerMac_)) {
    esp_now_del_peer(peerMac_);
  }
  if (esp_now_add_peer(&peer) != ESP_OK) {
    util::copyCString(lastError_, sizeof(lastError_), "esp_now_add_peer_failed");
    return false;
  }

  ready_ = true;
  Serial.printf("ESP-NOW TX peer %02X:%02X:%02X:%02X:%02X:%02X ch=%u enc=%u heap=%u\n",
                peerMac_[0], peerMac_[1], peerMac_[2], peerMac_[3], peerMac_[4], peerMac_[5],
                channel_, encrypt_ ? 1u : 0u, ESP.getFreeHeap());
  return true;
#else
  (void)wifiSsid;
  (void)peerMac;
  (void)pmk;
  store_ = &store;
  channel_ = fixedChannel >= 1 ? fixedChannel : 1;
  ready_ = true;
  return true;
#endif
}

void EspNowTelemetryPublisher::onReceiveTrampoline(const uint8_t* mac, const uint8_t* data,
                                                   int len) {
  if (instance_ != nullptr) {
    instance_->onReceive(mac, data, len);
  }
}

void EspNowTelemetryPublisher::onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  (void)mac;
  if (store_ == nullptr || data == nullptr ||
      len < static_cast<int>(sizeof(xt369p_bridge::EspNowCommandV1))) {
    return;
  }
  xt369p_bridge::EspNowCommandV1 command = {};
  std::memcpy(&command, data, sizeof(command));
  if (command.magic != xt369p_bridge::kCmdMagic || command.version != xt369p_bridge::kVersion) {
    return;
  }
  switch (command.command) {
    case static_cast<uint8_t>(meter::MeterCommand::ResetWh):
      store_->enqueueMeterCommand(meter::MeterCommand::ResetWh);
      break;
    case static_cast<uint8_t>(meter::MeterCommand::ResetAh):
      store_->enqueueMeterCommand(meter::MeterCommand::ResetAh);
      break;
    case static_cast<uint8_t>(meter::MeterCommand::ResetDuration):
      store_->enqueueMeterCommand(meter::MeterCommand::ResetDuration);
      break;
    case static_cast<uint8_t>(meter::MeterCommand::ResetAll):
      store_->enqueueMeterCommand(meter::MeterCommand::ResetAll);
      break;
    default:
      break;
  }
}

bool EspNowTelemetryPublisher::isReady() const {
  return ready_;
}

uint8_t EspNowTelemetryPublisher::channel() const {
  return channel_;
}

void EspNowTelemetryPublisher::publish(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  if (!ready_) {
    return;
  }

  const auto meter = store.meter();
  const auto status = store.status();
  const auto esp = store.espHealth();

  xt369p_bridge::EspNowPacketV1 packet = {};
  packet.magic = xt369p_bridge::kMagic;
  packet.version = xt369p_bridge::kVersion;
  if (status.sppConnected) {
    packet.flags |= xt369p_bridge::kFlagSppConnected;
  }
  if (meter.valid) {
    packet.flags |= xt369p_bridge::kFlagMeterValid;
  }
  if (meter.checksumOk) {
    packet.flags |= xt369p_bridge::kFlagChecksumOk;
  }
  if (encrypt_) {
    packet.flags |= xt369p_bridge::kFlagEncrypted;
  }
  packet.voltageCv = static_cast<uint16_t>(meter.voltageV * 100.0f + 0.5f);
  packet.currentMa = static_cast<int32_t>(meter.currentA * 1000.0f);
  packet.powerCw = static_cast<int32_t>(meter.powerW * 100.0f);
  packet.capacityCAh = static_cast<uint32_t>(meter.capacityAh * 100.0f + 0.5f);
  packet.energyCWh = static_cast<uint32_t>(meter.energyWh * 100.0f + 0.5f);
  packet.tempDc = static_cast<int16_t>(meter.temperatureC * 10.0f);
  packet.runtimeS = meter.runtimeS;
  packet.frameCount = meter.frameCount;
  packet.seq = ++seq_;
  util::copyCString(packet.sppTarget, sizeof(packet.sppTarget), status.sppTarget);
  util::copyCString(packet.lastError, sizeof(packet.lastError), status.lastError);
  packet.cpu0Percent = esp.cpuCore0Percent;
  packet.cpu1Percent = esp.cpuCore1Percent;
  packet.chipTempDc = static_cast<int16_t>(esp.chipTemperatureC * 10.0f);
  packet.heapKb = static_cast<uint16_t>(esp.freeHeapBytes / 1024u);
  packet.uptimeSec = esp.uptimeSeconds;

  const esp_err_t err =
      esp_now_send(peerMac_, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
  if (err != ESP_OK) {
    std::snprintf(lastError_, sizeof(lastError_), "esp_now_send_%d", static_cast<int>(err));
  }
#else
  (void)store;
#endif
}

const char* EspNowTelemetryPublisher::lastError() const {
  return lastError_;
}

}  // namespace xt369p::espnow_tx
