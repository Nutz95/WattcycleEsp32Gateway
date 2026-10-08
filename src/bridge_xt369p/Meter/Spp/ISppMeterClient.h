#pragma once

#include <cstddef>
#include <cstdint>

#include "Meter/Models/WattmeterTelemetry.h"
#include "Meter/Protocol/AtorchCommands.h"

namespace xt369p::meter {

class ISppMeterClient {
 public:
  virtual ~ISppMeterClient() = default;

  virtual bool beginAsMaster(const char* localName) = 0;
  virtual bool connectAddress(const char* btAddress) = 0;
  virtual bool connectName(const char* remoteName) = 0;
  /// Inquiry scan, log peers, connect to the first device whose name matches.
  virtual bool discoverAndConnect(const char* remoteName, uint32_t timeoutMs) = 0;
  virtual void disconnect() = 0;
  virtual bool isConnected() const = 0;
  virtual bool poll(WattmeterTelemetry& out) = 0;
  virtual bool sendCommand(MeterCommand command) = 0;
  virtual const char* lastError() const = 0;
  virtual const char* peerAddress() const = 0;
};

}  // namespace xt369p::meter
