#include "Bms/Ble/WattcycleBleClient.h"


#include "Bms/Protocol/AnalogQuantityParser.h"
#include "Bms/Protocol/FrameBuilder.h"
#include "Bms/Protocol/ProductInfoParser.h"
#include "Bms/Protocol/WarningInfoParser.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include <cstring>
#include <string>

namespace wattcycle::bms {
namespace {

WattcycleBleClient* gActiveClient = nullptr;
LoopYieldFn gLoopYield = nullptr;

void notifyCallback(NimBLERemoteCharacteristic* /*characteristic*/, uint8_t* data,
                    size_t length, bool /*isNotify*/) {
  if (gActiveClient != nullptr && data != nullptr) {
    gActiveClient->handleNotification(data, length);
  }
}

NimBLEClient* asClient(void* handle) {
  return static_cast<NimBLEClient*>(handle);
}

NimBLERemoteCharacteristic* asCharacteristic(void* handle) {
  return static_cast<NimBLERemoteCharacteristic*>(handle);
}

void pumpGatewayLoop() {
  if (gLoopYield != nullptr) {
    gLoopYield();
  }
}

int addressTypeSeenInScan(NimBLEScan* scan, const char* address) {
#if defined(WATTCYCLE_NIMBLE_V2)
  const NimBLEScanResults results = scan->getResults();
  for (int i = 0; i < results.getCount(); ++i) {
    const NimBLEAdvertisedDevice* device = results.getDevice(i);
    if (device == nullptr) {
      continue;
    }
    if (strcasecmp(device->getAddress().toString().c_str(), address) == 0) {
      return device->getAddress().getType();
    }
  }
#else
  // NimBLE 1 returns devices by value and its getters are non-const.
  NimBLEScanResults results = scan->getResults();
  for (int i = 0; i < results.getCount(); ++i) {
    NimBLEAdvertisedDevice device = results.getDevice(i);
    if (strcasecmp(device.getAddress().toString().c_str(), address) == 0) {
      return device.getAddress().getType();
    }
  }
#endif
  return -1;
}

}  // namespace

void WattcycleBleClient::setLoopYield(LoopYieldFn yieldFn) {
  gLoopYield = yieldFn;
}

WattcycleBleClient::WattcycleBleClient(int8_t txPower) : txPower_(txPower) {}

WattcycleBleClient::~WattcycleBleClient() {
  disconnect();
}

bool WattcycleBleClient::connect(const char* address, uint32_t timeoutMs) {
  if (address == nullptr || address[0] == '\0') {
    return false;
  }

  disconnect();
  NimBLEDevice::init("WattcycleGateway");
#if defined(WATTCYCLE_NIMBLE_V2)
  NimBLEDevice::setPower(txPower_);
#else
  NimBLEDevice::setPower(static_cast<esp_power_level_t>(txPower_));
#endif
  NimBLEDevice::setMTU(247);

  // Brief scan resolves the real address type (random vs public) before connect.
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(45);
  scan->setWindow(15);
#if defined(WATTCYCLE_NIMBLE_V2)
  scan->start(2500, false, true);
#else
  scan->start(3, false);
#endif

  // Prefer the type seen in the scan. Guessing the wrong type first burns a full timeout.
  uint8_t addressTypes[2] = {BLE_ADDR_RANDOM, BLE_ADDR_PUBLIC};
  uint8_t typeCount = 2;
  const int scannedType = addressTypeSeenInScan(scan, address);
  if (scannedType >= 0) {
    addressTypes[0] = static_cast<uint8_t>(scannedType);
    typeCount = 1;
    Serial.printf("BLE scan matched %s type=%u\n", address, addressTypes[0]);
  }
  scan->clearResults();

  NimBLEClient* client = nullptr;
  for (uint8_t typeIndex = 0; typeIndex < typeCount; ++typeIndex) {
    const uint8_t addressType = addressTypes[typeIndex];
    client = NimBLEDevice::createClient();
#if defined(WATTCYCLE_NIMBLE_V2)
    client->setConnectTimeout(timeoutMs < 1000 ? 1000 : timeoutMs);
    Serial.printf("BLE connect %s type=%u...\n", address, addressType);
    const bool connected =
        client->connect(NimBLEAddress(std::string(address), addressType));
#else
    const uint32_t timeoutSeconds =
        timeoutMs < 1000 ? 1 : (timeoutMs + 999) / 1000;
    client->setConnectTimeout(timeoutSeconds);
    Serial.printf("BLE connect %s type=%u...\n", address, addressType);
    const bool connected = client->connect(NimBLEAddress(address, addressType));
#endif
    if (connected) {
      Serial.printf("BLE connected (type=%u)\n", addressType);
      break;
    }
    NimBLEDevice::deleteClient(client);
    client = nullptr;
  }
  if (client == nullptr) {
    return false;
  }

  clientHandle_ = client;
  if (!resolveCharacteristics()) {
    Serial.println(F("BLE GATT resolve failed"));
    disconnect();
    return false;
  }

  auto* notify = asCharacteristic(notifyCharacteristic_);
#if defined(WATTCYCLE_NIMBLE_V2)
  const bool subscribed = notify->subscribe(true, notifyCallback, true);
#else
  const bool subscribed = notify->subscribe(true, notifyCallback);
#endif
  if (!subscribed) {
    Serial.println(F("BLE notify subscribe failed"));
    disconnect();
    return false;
  }

  gActiveClient = this;
  connected_ = true;
  if (!authenticate()) {
    Serial.println(F("BLE HiLink auth failed"));
    disconnect();
    return false;
  }
  delay(500);
  return true;
}

void WattcycleBleClient::disconnect() {
  connected_ = false;
  gActiveClient = nullptr;
  writeCharacteristic_ = nullptr;
  notifyCharacteristic_ = nullptr;
  authCharacteristic_ = nullptr;

  if (clientHandle_ != nullptr) {
    NimBLEClient* client = asClient(clientHandle_);
    if (client->isConnected()) {
      client->disconnect();
    }
    NimBLEDevice::deleteClient(client);
    clientHandle_ = nullptr;
  }
}

bool WattcycleBleClient::isConnected() const {
  if (!connected_ || clientHandle_ == nullptr) {
    return false;
  }
  return asClient(clientHandle_)->isConnected();
}

bool WattcycleBleClient::resolveCharacteristics() {
  NimBLEClient* client = asClient(clientHandle_);
  NimBLERemoteService* service = client->getService(kServiceUuid);
  if (service == nullptr) {
    return false;
  }

  writeCharacteristic_ = service->getCharacteristic(kWriteUuid);
  notifyCharacteristic_ = service->getCharacteristic(kNotifyUuid);
  authCharacteristic_ = service->getCharacteristic(kAuthUuid);
  return writeCharacteristic_ != nullptr && notifyCharacteristic_ != nullptr &&
         authCharacteristic_ != nullptr;
}

bool WattcycleBleClient::authenticate() {
  auto* auth = asCharacteristic(authCharacteristic_);
  return auth->writeValue(reinterpret_cast<const uint8_t*>(kAuthKey),
                          std::strlen(kAuthKey), false);
}

void WattcycleBleClient::handleNotification(const uint8_t* data, size_t length) {
  assembler_.append(data, length);
}

bool WattcycleBleClient::sendAndReceive(const uint8_t* request, size_t requestLength,
                                        uint32_t timeoutMs) {
  if (!isConnected() || request == nullptr || requestLength == 0) {
    return false;
  }

  assembler_.reset();
  auto* writeChar = asCharacteristic(writeCharacteristic_);
  if (!writeChar->writeValue(request, requestLength, false)) {
    return false;
  }

  const uint32_t startedAt = millis();
  while (!assembler_.isComplete()) {
    if (millis() - startedAt > timeoutMs) {
      return false;
    }
    pumpGatewayLoop();
    delay(10);
  }
  return true;
}

bool WattcycleBleClient::requestDataPoint(uint16_t dataPoint, ParsedFrame& outFrame) {
  uint8_t frame[16];
  const size_t length =
      FrameBuilder::buildReadFrame(dataPoint, frameHead_, frame, sizeof(frame));
  if (!sendAndReceive(frame, length, 5000)) {
    return false;
  }
  outFrame = FrameParser::parse(assembler_.data(), assembler_.size());
  return outFrame.valid;
}

bool WattcycleBleClient::detectFrameHead() {
  uint8_t frame[16];
  const uint8_t heads[2] = {kFrameHead, kFrameHeadAlt};
  for (uint8_t head : heads) {
    const size_t length =
        FrameBuilder::buildProductInfoRead(head, frame, sizeof(frame));
    if (length == 0) {
      continue;
    }
    if (sendAndReceive(frame, length, 3000)) {
      const auto parsed = FrameParser::parse(assembler_.data(), assembler_.size());
      if (parsed.valid) {
        frameHead_ = head;
        return true;
      }
    }
  }
  return false;
}

bool WattcycleBleClient::readAnalogQuantity(BatteryTelemetry& out) {
  ParsedFrame parsed;
  if (!requestDataPoint(kDpAnalogQuantity, parsed)) {
    return false;
  }
  return AnalogQuantityParser::parse(parsed.data, parsed.dataLength, out);
}

bool WattcycleBleClient::readProductInfo(ProductInfo& out) {
  ParsedFrame parsed;
  if (!requestDataPoint(kDpProductInfo, parsed)) {
    return false;
  }
  return ProductInfoParser::parse(parsed.data, parsed.dataLength, out);
}

bool WattcycleBleClient::readWarningFlags(WarningFlags& out) {
  ParsedFrame parsed;
  if (!requestDataPoint(kDpWarningInfo, parsed)) {
    return false;
  }
  return WarningInfoParser::parse(parsed.data, parsed.dataLength, out);
}

}  // namespace wattcycle::bms

