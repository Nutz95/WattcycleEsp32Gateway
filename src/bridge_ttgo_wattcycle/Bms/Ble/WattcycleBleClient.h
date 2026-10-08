#pragma once

#include "Bms/Ble/BlePacketAssembler.h"
#include "Bms/Ble/IBmsBleClient.h"
#include "Bms/Protocol/FrameParser.h"

namespace wattcycle::bms {

using LoopYieldFn = void (*)();

class WattcycleBleClient : public IBmsBleClient {
 public:
  WattcycleBleClient();
  ~WattcycleBleClient() override;

  static void setLoopYield(LoopYieldFn yieldFn);

  bool connect(const char* address, uint32_t timeoutMs) override;
  void disconnect() override;
  bool isConnected() const override;
  bool detectFrameHead() override;
  bool readAnalogQuantity(BatteryTelemetry& out) override;
  bool readProductInfo(ProductInfo& out) override;
  bool readWarningFlags(WarningFlags& out) override;

  void handleNotification(const uint8_t* data, size_t length);

 private:
  bool sendAndReceive(const uint8_t* request, size_t requestLength, uint32_t timeoutMs);
  bool authenticate();
  bool resolveCharacteristics();
  bool requestDataPoint(uint16_t dataPoint, ParsedFrame& outFrame);

  BlePacketAssembler assembler_;
  uint8_t frameHead_ = kFrameHead;
  bool connected_ = false;
  void* clientHandle_ = nullptr;
  void* writeCharacteristic_ = nullptr;
  void* notifyCharacteristic_ = nullptr;
  void* authCharacteristic_ = nullptr;
};

}  // namespace wattcycle::bms
