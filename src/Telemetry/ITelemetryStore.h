#pragma once

#include "Bms/Models/BatteryTelemetry.h"
#include "Telemetry/GatewayStatus.h"

namespace wattcycle::telemetry {

/// Read/write boundary for the latest BMS snapshot and gateway status.
class ITelemetryStore {
 public:
  virtual ~ITelemetryStore() = default;

  virtual void updateBattery(const bms::BatteryTelemetry& telemetry) = 0;
  virtual void updateProduct(const bms::ProductInfo& product) = 0;
  virtual void updateWarnings(const bms::WarningFlags& warnings) = 0;
  virtual void setWifiState(bool connected, const char* ipAddress) = 0;
  virtual void setBleState(bool connected, const char* address, const char* error) = 0;
  virtual void setTelemetryFresh(bool fresh) = 0;

  virtual bms::BatteryTelemetry battery() const = 0;
  virtual bms::ProductInfo product() const = 0;
  virtual bms::WarningFlags warnings() const = 0;
  virtual GatewayStatus status() const = 0;
};

}  // namespace wattcycle::telemetry
