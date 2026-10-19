#pragma once

#include "Bms/Models/BatteryTelemetry.h"

namespace wattcycle::bms {

/// BLE transport for Wattcycle / XDZN BMS devices.
class IBmsBleClient {
 public:
  virtual ~IBmsBleClient() = default;

  /// Connect to the BMS at the given BLE address.
  virtual bool connect(const char* address, uint32_t timeoutMs) = 0;
  /// Drop the active BLE session.
  virtual void disconnect() = 0;
  /// True while a session is open.
  virtual bool isConnected() const = 0;
  /// Probe for a valid BMS frame head on the notify path.
  virtual bool detectFrameHead() = 0;
  /// Read pack voltage / current / SoC / temps into out.
  virtual bool readAnalogQuantity(BatteryTelemetry& out) = 0;
  /// Read product / serial metadata into out.
  virtual bool readProductInfo(ProductInfo& out) = 0;
  /// Read protection / fault bitfields into out.
  virtual bool readWarningFlags(WarningFlags& out) = 0;
};

}  // namespace wattcycle::bms
