#include "EcoFlow/Ble/EcoFlowBleGatt.h"

#include "Config/TimingConstants.h"
#include "EcoFlow/Ble/EcoFlowBleUuids.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include <string>

namespace ecoflow::ble {
namespace {

using ecoflow::config::TimingConstants;

class EcoFlowClientGapCallbacks : public NimBLEClientCallbacks {
 public:
  void onConnect(NimBLEClient* /*client*/) override {
    Serial.println("[ecoflow-ble] GAP connected");
  }

  void onDisconnect(NimBLEClient* /*client*/, int reason) override {
    Serial.printf("[ecoflow-ble] GAP disconnected reason=%d\n", reason);
  }

  bool onConnParamsUpdateRequest(NimBLEClient* /*client*/,
                                 const ble_gap_upd_params* params) override {
    if (params != nullptr) {
      Serial.printf(
          "[ecoflow-ble] peer L2CAP conn-update itvl=%u..%u lat=%u to=%u — accept\n",
          static_cast<unsigned>(params->itvl_min),
          static_cast<unsigned>(params->itvl_max),
          static_cast<unsigned>(params->latency),
          static_cast<unsigned>(params->supervision_timeout));
    }
    return true;
  }
};

EcoFlowBleGatt* gGattNotifyOwner = nullptr;
EcoFlowClientGapCallbacks gGapCallbacks;

void onGattNotify(NimBLERemoteCharacteristic* /*characteristic*/, uint8_t* data, size_t length,
                  bool /*isNotify*/) {
  if (gGattNotifyOwner == nullptr || data == nullptr || length == 0) {
    return;
  }
  gGattNotifyOwner->dispatchNotify(data, length);
}

NimBLEAddress gTargetAddress;
NimBLEAdvertisedDevice gFoundDevice;
volatile bool gFoundFlag = false;

class EcoFlowScanCallbacks : public NimBLEScanCallbacks {
 public:
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    if (advertisedDevice == nullptr || gFoundFlag) {
      return;
    }
    if (!advertisedDevice->getAddress().equals(gTargetAddress)) {
      return;
    }
    gFoundDevice = *advertisedDevice;
    gFoundFlag = true;
    NimBLEDevice::getScan()->stop();
  }
};

EcoFlowScanCallbacks gScanCallbacks;

}  // namespace

EcoFlowBleGatt::EcoFlowBleGatt() = default;

EcoFlowBleGatt::~EcoFlowBleGatt() {
  disconnect();
}

void EcoFlowBleGatt::dispatchNotify(const uint8_t* data, size_t length) const {
  if (notifyFn_ == nullptr || notifyOwner_ == nullptr || data == nullptr || length == 0) {
    return;
  }
  notifyFn_(notifyOwner_, data, length);
}

bool EcoFlowBleGatt::connectFromScan(const char* bleAddress, uint8_t& outAddressType,
                                     bool& outHaveType) {
  gTargetAddress = NimBLEAddress(std::string(bleAddress), BLE_ADDR_PUBLIC);
  gFoundFlag = false;

  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(&gScanCallbacks, false);
  scan->setActiveScan(false);
  scan->setDuplicateFilter(true);
  scan->setInterval(TimingConstants::kBleScanIntervalSlots);
  scan->setWindow(TimingConstants::kBleScanWindowSlots);
  scan->clearResults();

  Serial.println("[ecoflow-ble] scanning for peer (targeted, passive)…");
  const uint32_t scanStartMs = millis();
  scan->start(TimingConstants::kBleScanDurationMs, false, true);
  while (!gFoundFlag && (millis() - scanStartMs) < TimingConstants::kBleScanDurationMs) {
    delay(20);
  }
  scan->stop();
  scan->clearResults();
  scan->setScanCallbacks(nullptr, false);

  if (!gFoundFlag) {
    return false;
  }

  outAddressType = gFoundDevice.getAddressType();
  outHaveType = true;
  Serial.printf("[ecoflow-ble] found in scan type=%u rssi=%d\n",
                static_cast<unsigned>(outAddressType), gFoundDevice.getRSSI());
  delay(TimingConstants::kBleScanStopSettleMs);

  NimBLEClient* client = NimBLEDevice::createClient();
  if (client == nullptr) {
    return false;
  }
  clientHandle_ = client;
  configureClientLink();

  if (!client->connect(&gFoundDevice, false, false, false)) {
    Serial.printf("[ecoflow-ble] connect(scan) failed lastErr=%d\n", client->getLastError());
    NimBLEDevice::deleteClient(client);
    clientHandle_ = nullptr;
    return false;
  }
  Serial.println("[ecoflow-ble] connected via scan device (no MTU exchange)");
  return true;
}

bool EcoFlowBleGatt::connectByAddress(const char* bleAddress, bool haveType, uint8_t addressType) {
  NimBLEClient* client = NimBLEDevice::createClient();
  if (client == nullptr) {
    return false;
  }
  clientHandle_ = client;
  configureClientLink();

  const uint8_t type = haveType ? addressType : BLE_ADDR_PUBLIC;
  NimBLEAddress address(std::string(bleAddress), type);
  if (!client->connect(address, false, false, false)) {
    NimBLEDevice::deleteClient(client);
    clientHandle_ = nullptr;
    return false;
  }
  Serial.printf("[ecoflow-ble] connected via address type=%u (no MTU exchange)\n",
                static_cast<unsigned>(type));
  return true;
}

void EcoFlowBleGatt::configureClientLink() const {
  auto* client = static_cast<NimBLEClient*>(clientHandle_);
  client->setClientCallbacks(&gGapCallbacks, false);
  client->setConnectTimeout(TimingConstants::kBleConnectTimeoutMs);
  client->setConnectionParams(TimingConstants::kBleConnIntervalMin,
                              TimingConstants::kBleConnIntervalMax,
                              TimingConstants::kBleConnLatency,
                              TimingConstants::kBleConnSupervisionTimeout,
                              TimingConstants::kBleConnScanIntervalSlots,
                              TimingConstants::kBleConnScanWindowSlots);
}

bool EcoFlowBleGatt::resolveCharacteristics(EcoFlowNotifyFn notifyFn, void* notifyOwner) {
  auto* client = static_cast<NimBLEClient*>(clientHandle_);
  if (client == nullptr || !client->isConnected()) {
    return false;
  }

  notifyFn_ = notifyFn;
  notifyOwner_ = notifyOwner;
  gGattNotifyOwner = this;

  for (uint8_t attempt = 1; attempt <= TimingConstants::kBleGattDiscoverAttempts; ++attempt) {
    if (!client->isConnected()) {
      return false;
    }

    Serial.printf("[ecoflow-ble] gatt uuid lookup attempt %u/%u\n",
                  static_cast<unsigned>(attempt),
                  static_cast<unsigned>(TimingConstants::kBleGattDiscoverAttempts));

    NimBLERemoteService* service = client->getService(EcoFlowBleUuids::kService);
    if (service == nullptr) {
      Serial.printf("[ecoflow-ble] getService failed err=%d\n", client->getLastError());
      delay(TimingConstants::kBleGattRetryDelayMs);
      continue;
    }

    NimBLERemoteCharacteristic* writeChar = service->getCharacteristic(EcoFlowBleUuids::kWrite);
    NimBLERemoteCharacteristic* notifyChar = service->getCharacteristic(EcoFlowBleUuids::kNotify);
    if (writeChar == nullptr || notifyChar == nullptr || !notifyChar->canNotify()) {
      delay(TimingConstants::kBleGattRetryDelayMs);
      continue;
    }
    if (!notifyChar->subscribe(true, onGattNotify, true)) {
      Serial.println("[ecoflow-ble] subscribe failed");
      delay(TimingConstants::kBleGattRetryDelayMs);
      continue;
    }

    writeCharacteristic_ = writeChar;
    notifyCharacteristic_ = notifyChar;
    Serial.println("[ecoflow-ble] GATT write+notify ready (filtered discovery)");
    return true;
  }
  return false;
}

bool EcoFlowBleGatt::write(const uint8_t* data, size_t length) {
  auto* writeChar = static_cast<NimBLERemoteCharacteristic*>(writeCharacteristic_);
  if (writeChar == nullptr || data == nullptr || length == 0) {
    return false;
  }
  return writeChar->writeValue(data, length, false);
}

void EcoFlowBleGatt::disconnect() {
  writeCharacteristic_ = nullptr;
  notifyCharacteristic_ = nullptr;
  auto* client = static_cast<NimBLEClient*>(clientHandle_);
  if (client != nullptr) {
    if (client->isConnected()) {
      client->disconnect();
      delay(TimingConstants::kBleDisconnectSettleMs);
    }
    NimBLEDevice::deleteClient(client);
    clientHandle_ = nullptr;
  }
  if (gGattNotifyOwner == this) {
    gGattNotifyOwner = nullptr;
  }
}

bool EcoFlowBleGatt::isLinkUp() const {
  auto* client = static_cast<NimBLEClient*>(clientHandle_);
  return client != nullptr && client->isConnected();
}

}  // namespace ecoflow::ble
