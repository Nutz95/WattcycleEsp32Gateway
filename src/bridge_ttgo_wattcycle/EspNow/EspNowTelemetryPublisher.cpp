#include "EspNow/EspNowTelemetryPublisher.h"

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

namespace wattcycle::espnow_tx {
namespace {

int16_t toDeciCelsius(float celsius) {
  return static_cast<int16_t>(celsius * 10.0f);
}

}  // namespace

uint8_t EspNowTelemetryPublisher::resolveChannel(const char* wifiSsid,
                                                 uint8_t fixedChannel) const {
#ifndef UNIT_TEST
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
#else
  (void)wifiSsid;
  return fixedChannel >= 1 ? fixedChannel : 1;
#endif
}

bool EspNowTelemetryPublisher::begin(const char* wifiSsid, const char* peerMac,
                                     uint8_t fixedChannel, const char* pmk) {
#ifndef UNIT_TEST
  ready_ = false;
  channel_ = 0;
  lastError_[0] = '\0';

  if (!wattcycle::util::parseMacAddress(peerMac, peerMac_)) {
    wattcycle::util::copyCString(lastError_, sizeof(lastError_), "bad_peer_mac");
    return false;
  }

  encrypt_ = pmk != nullptr && std::strlen(pmk) >= wattcycle_bridge::kPmkBytes;

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  WiFi.setSleep(WIFI_PS_MIN_MODEM);
  Serial.printf("BMS bridge STA MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println(F("  -> set as ESPNOW_BMS_BRIDGE_MAC on the M5 hub"));

  channel_ = resolveChannel(wifiSsid, fixedChannel);
  if (esp_wifi_set_channel(channel_, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    wattcycle::util::copyCString(lastError_, sizeof(lastError_), "set_channel_failed");
    return false;
  }

  if (esp_now_init() != ESP_OK) {
    wattcycle::util::copyCString(lastError_, sizeof(lastError_), "esp_now_init_failed");
    return false;
  }

  if (encrypt_) {
    uint8_t key[wattcycle_bridge::kPmkBytes] = {};
    std::memcpy(key, pmk, wattcycle_bridge::kPmkBytes);
    if (esp_now_set_pmk(key) != ESP_OK) {
      wattcycle::util::copyCString(lastError_, sizeof(lastError_), "esp_now_set_pmk_failed");
      return false;
    }
  }

  esp_now_peer_info_t peer = {};
  std::memcpy(peer.peer_addr, peerMac_, 6);
  peer.channel = channel_;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = encrypt_;
  if (encrypt_) {
    std::memcpy(peer.lmk, pmk, wattcycle_bridge::kPmkBytes);
  }
  if (esp_now_is_peer_exist(peerMac_)) {
    esp_now_del_peer(peerMac_);
  }
  if (esp_now_add_peer(&peer) != ESP_OK) {
    wattcycle::util::copyCString(lastError_, sizeof(lastError_), "esp_now_add_peer_failed");
    return false;
  }

  ready_ = true;
  Serial.printf("ESP-NOW TX peer %02X:%02X:%02X:%02X:%02X:%02X ch=%u enc=%u\n", peerMac_[0],
                peerMac_[1], peerMac_[2], peerMac_[3], peerMac_[4], peerMac_[5], channel_,
                encrypt_ ? 1u : 0u);
  return true;
#else
  (void)wifiSsid;
  (void)peerMac;
  (void)pmk;
  channel_ = fixedChannel >= 1 ? fixedChannel : 1;
  ready_ = true;
  return true;
#endif
}

bool EspNowTelemetryPublisher::isReady() const {
  return ready_;
}

uint8_t EspNowTelemetryPublisher::channel() const {
  return channel_;
}

void EspNowTelemetryPublisher::publishProduct(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto product = store.product();
  const auto status = store.status();
  wattcycle_bridge::EspNowProductPacketV1 packet = {};
  packet.magic = wattcycle_bridge::kProductMagic;
  packet.version = wattcycle_bridge::kVersion;
  if (encrypt_) {
    packet.flags |= wattcycle_bridge::kFlagEncrypted;
  }
  packet.seq = ++productSeq_;
  wattcycle::util::copyCString(packet.firmwareVersion, sizeof(packet.firmwareVersion),
                               product.firmwareVersion);
  wattcycle::util::copyCString(packet.manufacturerName, sizeof(packet.manufacturerName),
                               product.manufacturerName);
  wattcycle::util::copyCString(packet.serialNumber, sizeof(packet.serialNumber),
                               product.serialNumber);
  wattcycle::util::copyCString(packet.bleAddress, sizeof(packet.bleAddress), status.bleAddress);
  wattcycle::util::copyCString(packet.lastError, sizeof(packet.lastError), status.lastError);
  const esp_err_t err =
      esp_now_send(peerMac_, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
  if (err != ESP_OK) {
    std::snprintf(lastError_, sizeof(lastError_), "esp_now_prod_%d", static_cast<int>(err));
  }
#else
  (void)store;
#endif
}

void EspNowTelemetryPublisher::publish(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  if (!ready_) {
    return;
  }

  const auto battery = store.battery();
  const auto warnings = store.warnings();
  const auto status = store.status();
  const auto esp = store.espHealth();

  wattcycle_bridge::EspNowTelemetryPacketV1 packet = {};
  packet.magic = wattcycle_bridge::kMagic;
  packet.version = wattcycle_bridge::kVersion;
  if (status.bleConnected) {
    packet.flags |= wattcycle_bridge::kFlagBleConnected;
  }
  if (battery.valid) {
    packet.flags |= wattcycle_bridge::kFlagBatteryValid;
  }
  if (warnings.valid) {
    packet.flags |= wattcycle_bridge::kFlagWarningsValid;
  }
  if (encrypt_) {
    packet.flags |= wattcycle_bridge::kFlagEncrypted;
  }
  if (warnings.hasActiveProtection) {
    packet.flags |= wattcycle_bridge::kFlagHasProtection;
  }
  if (warnings.hasFault) {
    packet.flags |= wattcycle_bridge::kFlagHasFault;
  }
  packet.flags |= wattcycle_bridge::kFlagBridgeEspValid;
  packet.seq = ++seq_;
  packet.socPercent = static_cast<uint8_t>(battery.stateOfChargePercent);
  packet.sohPercent = static_cast<uint8_t>(battery.stateOfHealthPercent);
  packet.moduleVoltageCv = static_cast<uint16_t>(battery.moduleVoltage * 100.0f + 0.5f);
  packet.currentDa = static_cast<int16_t>(battery.currentAmps * 10.0f);
  packet.powerW = static_cast<int16_t>(battery.powerWatts);
  packet.balanceCurrentDa = static_cast<int16_t>(battery.balanceCurrentAmps * 10.0f);
  packet.remainingCAh = static_cast<uint16_t>(battery.remainingCapacityAh * 100.0f + 0.5f);
  packet.totalCAh = static_cast<uint16_t>(battery.totalCapacityAh * 100.0f + 0.5f);
  packet.designCAh = static_cast<uint16_t>(battery.designCapacityAh * 100.0f + 0.5f);
  packet.cycleNumber = battery.cycleNumber;
  packet.mosTempDc = toDeciCelsius(battery.mosTemperatureC);
  packet.pcbTempDc = toDeciCelsius(battery.pcbTemperatureC);

  const uint8_t cellCount =
      battery.cellCount > wattcycle_bridge::kMaxCells
          ? static_cast<uint8_t>(wattcycle_bridge::kMaxCells)
          : battery.cellCount;
  packet.cellCount = cellCount;
  for (uint8_t i = 0; i < cellCount; ++i) {
    packet.cellMv[i] = static_cast<uint16_t>(battery.cellVoltages[i] * 1000.0f + 0.5f);
    if (warnings.cellBalancing[i]) {
      packet.balanceBits[i / 8u] |= static_cast<uint8_t>(1u << (i % 8u));
    }
  }
  packet.statusRegister1 = warnings.statusRegister1;
  packet.statusRegister2 = warnings.statusRegister2;
  packet.statusRegister5 = warnings.statusRegister5;
  packet.warningRegister1 = warnings.warningRegister1;
  packet.warningRegister2 = warnings.warningRegister2;
  packet.cpu0Percent = esp.cpuCore0Percent;
  packet.cpu1Percent = esp.cpuCore1Percent;
  packet.chipTempDc = toDeciCelsius(esp.chipTemperatureC);
  packet.heapKb = static_cast<uint16_t>(esp.freeHeapBytes / 1024u);
  packet.uptimeSec = esp.uptimeSeconds;

  const esp_err_t err =
      esp_now_send(peerMac_, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
  if (err != ESP_OK) {
    std::snprintf(lastError_, sizeof(lastError_), "esp_now_send_%d", static_cast<int>(err));
  }

  const uint32_t nowMs = millis();
  if (lastProductPublishMs_ == 0 || (nowMs - lastProductPublishMs_) >= 15000u) {
    lastProductPublishMs_ = nowMs;
    publishProduct(store);
  }
#else
  (void)store;
#endif
}

const char* EspNowTelemetryPublisher::lastError() const {
  return lastError_;
}

}  // namespace wattcycle::espnow_tx
