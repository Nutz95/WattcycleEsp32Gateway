#include "Bms/Ble/WattcycleBleClient.h"

#ifndef UNIT_TEST

#include "Bms/Protocol/AnalogQuantityParser.h"
#include "Bms/Protocol/FrameBuilder.h"
#include "Bms/Protocol/ProductInfoParser.h"
#include "Bms/Protocol/WarningInfoParser.h"

#include <NimBLEDevice.h>
#include <cstring>

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

}  // namespace

void WattcycleBleClient::setLoopYield(LoopYieldFn yieldFn) {
  gLoopYield = yieldFn;
}

WattcycleBleClient::WattcycleBleClient() = default;

WattcycleBleClient::~WattcycleBleClient() {
  disconnect();
}

bool WattcycleBleClient::connect(const char* address, uint32_t timeoutMs) {
  if (address == nullptr || address[0] == '\0') {
    return false;
  }

  disconnect();
  NimBLEDevice::init("WattcycleGateway");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  NimBLEClient* client = NimBLEDevice::createClient();
  client->setConnectTimeout(timeoutMs / 1000);
  if (!client->connect(NimBLEAddress(address))) {
    NimBLEDevice::deleteClient(client);
    return false;
  }

  clientHandle_ = client;
  if (!resolveCharacteristics()) {
    disconnect();
    return false;
  }

  auto* notify = asCharacteristic(notifyCharacteristic_);
  if (!notify->subscribe(true, notifyCallback)) {
    disconnect();
    return false;
  }

  gActiveClient = this;
  connected_ = true;
  if (!authenticate()) {
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

#endif
