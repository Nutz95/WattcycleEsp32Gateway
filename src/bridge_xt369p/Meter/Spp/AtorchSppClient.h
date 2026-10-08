#pragma once

#include "Meter/Protocol/AtorchFrameParser.h"
#include "Meter/Spp/ISppMeterClient.h"

#ifndef UNIT_TEST
#include <BluetoothSerial.h>
#endif

namespace xt369p::meter {

class AtorchSppClient : public ISppMeterClient {
 public:
  bool beginAsMaster(const char* localName) override;
  bool connectAddress(const char* btAddress) override;
  bool connectName(const char* remoteName) override;
  bool discoverAndConnect(const char* remoteName, uint32_t timeoutMs) override;
  void disconnect() override;
  bool isConnected() const override;
  bool poll(WattmeterTelemetry& out) override;
  bool sendCommand(MeterCommand command) override;
  const char* lastError() const override;
  const char* peerAddress() const override;

 private:
  static bool parseBtAddress(const char* text, uint8_t out[6]);
  static bool looksLikeBogusAddress(const uint8_t addr[6]);

#ifndef UNIT_TEST
  BluetoothSerial serialBt_;
#endif
  AtorchFrameParser parser_{};
  bool started_ = false;
  bool connected_ = false;
  char lastError_[64] = {};
  char peerAddress_[18] = {};
};

}  // namespace xt369p::meter
