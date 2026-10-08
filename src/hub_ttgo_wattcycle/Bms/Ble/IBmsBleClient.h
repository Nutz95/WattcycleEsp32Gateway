#pragma once

#include "Bms/Models/BatteryTelemetry.h"

namespace wattcycle::bms {

/// BLE transport for Wattcycle / XDZN BMS devices.
class IBmsBleClient {
 public:
  virtual ~IBmsBleClient() = default;

  virtual bool connect(const char* address, uint32_t timeoutMs) = 0;
  virtual void disconnect() = 0;
  virtual bool isConnected() const = 0;
  virtual bool detectFrameHead() = 0;
  virtual bool readAnalogQuantity(BatteryTelemetry& out) = 0;
  virtual bool readProductInfo(ProductInfo& out) = 0;
  virtual bool readWarningFlags(WarningFlags& out) = 0;
};

}  // namespace wattcycle::bms
