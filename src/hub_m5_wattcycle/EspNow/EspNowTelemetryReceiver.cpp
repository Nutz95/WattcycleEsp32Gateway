#include "EspNow/EspNowTelemetryReceiver.h"

#include "Telemetry/SolarBridgeTelemetry.h"
#include "Util/MacAddress.h"
#include "Util/SafeCopy.h"

#include <cstddef>
#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <esp_now.h>
#endif

namespace wattcycle::espnow_rx {

EspNowTelemetryReceiver* EspNowTelemetryReceiver::instance_ = nullptr;

EspNowTelemetryReceiver::EspNowTelemetryReceiver(telemetry::ITelemetryStore& store)
    : store_(store) {}

bool EspNowTelemetryReceiver::begin(const char* bmsBridgeMac, const char* xtBridgeMac,
                                    const char* pmk) {
#ifndef UNIT_TEST
  instance_ = this;
  ready_ = false;

  uint8_t bmsMac[6] = {};
  uint8_t xtMac[6] = {};
  const bool haveBms = util::parseMacAddress(bmsBridgeMac, bmsMac);
  const bool haveXt = util::parseMacAddress(xtBridgeMac, xtMac);
  if (!haveBms && !haveXt) {
    Serial.println(F("ESP-NOW skipped (set ESPNOW_BMS_BRIDGE_MAC and/or ESPNOW_BRIDGE_MAC)"));
    return false;
  }

  encrypt_ = pmk != nullptr && std::strlen(pmk) >= xt369p_bridge::kPmkBytes;
  if (encrypt_) {
    std::memcpy(lmk_, pmk, xt369p_bridge::kPmkBytes);
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

  esp_now_register_recv_cb(&EspNowTelemetryReceiver::onReceiveTrampoline);
  ready_ = true;
  {
    telemetry::SolarBridgeTelemetry solar = store_.solar();
    solar.espNowEncrypted = encrypt_;
    store_.updateSolar(solar);
  }
  Serial.printf("ESP-NOW multi-peer RX (bms=%u xt=%u enc=%u)\n", haveBms ? 1u : 0u,
                haveXt ? 1u : 0u, encrypt_ ? 1u : 0u);
  return true;
#else
  (void)bmsBridgeMac;
  (void)xtBridgeMac;
  (void)pmk;
  ready_ = true;
  return true;
#endif
}

void EspNowTelemetryReceiver::loop() {
  if (pendingKind_ != PendingKind::None) {
    applyPending();
  }
#ifndef UNIT_TEST
  const auto solar = store_.solar();
  if (solar.receivedAtMs != 0 && (millis() - solar.receivedAtMs) > 5000u && solar.linkFresh) {
    telemetry::SolarBridgeTelemetry stale = solar;
    stale.linkFresh = false;
    store_.updateSolar(stale);
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
    pendingSolar_ = packet;
    if (mac != nullptr) {
      std::memcpy(xtPeerMac_, mac, 6);
      hasXtPeer_ = true;
    }
    pendingKind_ = PendingKind::Solar;
    return;
  }

  if (magic == wattcycle_bridge::kMagic && version == wattcycle_bridge::kVersion) {
    if (len < static_cast<int>(sizeof(wattcycle_bridge::EspNowTelemetryPacketV1))) {
      return;
    }
    std::memcpy(&pendingBms_, data, sizeof(pendingBms_));
    pendingKind_ = PendingKind::BmsTelemetry;
    return;
  }

  if (magic == wattcycle_bridge::kProductMagic && version == wattcycle_bridge::kVersion) {
    if (len < static_cast<int>(sizeof(wattcycle_bridge::EspNowProductPacketV1))) {
      return;
    }
    std::memcpy(&pendingProduct_, data, sizeof(pendingProduct_));
    pendingKind_ = PendingKind::BmsProduct;
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

void EspNowTelemetryReceiver::applySolar(const xt369p_bridge::EspNowPacketV1& packet) {
  telemetry::SolarBridgeTelemetry solar = {};
  solar.linkFresh = true;
  solar.sppConnected = (packet.flags & xt369p_bridge::kFlagSppConnected) != 0;
  solar.meterValid = (packet.flags & xt369p_bridge::kFlagMeterValid) != 0;
  solar.checksumOk = (packet.flags & xt369p_bridge::kFlagChecksumOk) != 0;
  solar.espNowEncrypted = encrypt_ || ((packet.flags & xt369p_bridge::kFlagEncrypted) != 0);
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
  if (packet.version >= 2) {
    solar.bridgeEspValid = true;
    solar.bridgeEsp.cpuCore0Percent = packet.cpu0Percent;
    solar.bridgeEsp.cpuCore1Percent = packet.cpu1Percent;
    solar.bridgeEsp.chipTemperatureC = packet.chipTempDc / 10.0f;
    solar.bridgeEsp.freeHeapBytes = static_cast<uint32_t>(packet.heapKb) * 1024u;
    solar.bridgeEsp.uptimeSeconds = packet.uptimeSec;
  }
  store_.updateSolar(solar);
}

void EspNowTelemetryReceiver::applyBmsTelemetry(
    const wattcycle_bridge::EspNowTelemetryPacketV1& packet) {
  bms::BatteryTelemetry battery = {};
  battery.valid = (packet.flags & wattcycle_bridge::kFlagBatteryValid) != 0;
  battery.stateOfChargePercent = packet.socPercent;
  battery.stateOfHealthPercent = packet.sohPercent;
  battery.moduleVoltage = packet.moduleVoltageCv / 100.0f;
  battery.currentAmps = packet.currentDa / 10.0f;
  battery.powerWatts = static_cast<float>(packet.powerW);
  battery.balanceCurrentAmps = packet.balanceCurrentDa / 10.0f;
  battery.remainingCapacityAh = packet.remainingCAh / 100.0f;
  battery.totalCapacityAh = packet.totalCAh / 100.0f;
  battery.designCapacityAh = packet.designCAh / 100.0f;
  battery.cycleNumber = packet.cycleNumber;
  battery.mosTemperatureC = packet.mosTempDc / 10.0f;
  battery.pcbTemperatureC = packet.pcbTempDc / 10.0f;
  battery.cellCount =
      packet.cellCount > wattcycle_bridge::kMaxCells ? wattcycle_bridge::kMaxCells : packet.cellCount;
  battery.temperatureCount = 2;
  for (uint8_t i = 0; i < battery.cellCount; ++i) {
    battery.cellVoltages[i] = packet.cellMv[i] / 1000.0f;
  }
#ifndef UNIT_TEST
  battery.updatedAtMs = millis();
#endif
  store_.updateBattery(battery);
  store_.setBleState((packet.flags & wattcycle_bridge::kFlagBleConnected) != 0, "", "");
  store_.setTelemetryFresh(battery.valid);

  bms::WarningFlags warnings = {};
  warnings.valid = (packet.flags & wattcycle_bridge::kFlagWarningsValid) != 0;
  warnings.cellCount = battery.cellCount;
  warnings.hasActiveProtection = (packet.flags & wattcycle_bridge::kFlagHasProtection) != 0;
  warnings.hasFault = (packet.flags & wattcycle_bridge::kFlagHasFault) != 0;
  warnings.statusRegister1 = packet.statusRegister1;
  warnings.statusRegister2 = packet.statusRegister2;
  warnings.statusRegister5 = packet.statusRegister5;
  warnings.warningRegister1 = packet.warningRegister1;
  warnings.warningRegister2 = packet.warningRegister2;
  for (uint8_t i = 0; i < battery.cellCount; ++i) {
    warnings.cellBalancing[i] = (packet.balanceBits[i / 8u] & (1u << (i % 8u))) != 0;
  }
  store_.updateWarnings(warnings);
}

void EspNowTelemetryReceiver::applyBmsProduct(
    const wattcycle_bridge::EspNowProductPacketV1& packet) {
  bms::ProductInfo product = {};
  product.valid = true;
  util::copyCString(product.firmwareVersion, sizeof(product.firmwareVersion),
                    packet.firmwareVersion);
  util::copyCString(product.manufacturerName, sizeof(product.manufacturerName),
                    packet.manufacturerName);
  util::copyCString(product.serialNumber, sizeof(product.serialNumber), packet.serialNumber);
  store_.updateProduct(product);
  store_.setBleState(true, packet.bleAddress, packet.lastError);
}

void EspNowTelemetryReceiver::applyPending() {
  const PendingKind kind = pendingKind_;
  pendingKind_ = PendingKind::None;
  switch (kind) {
    case PendingKind::Solar:
      applySolar(pendingSolar_);
      break;
    case PendingKind::BmsTelemetry:
      applyBmsTelemetry(pendingBms_);
      break;
    case PendingKind::BmsProduct:
      applyBmsProduct(pendingProduct_);
      break;
    case PendingKind::None:
      break;
  }
}

}  // namespace wattcycle::espnow_rx
