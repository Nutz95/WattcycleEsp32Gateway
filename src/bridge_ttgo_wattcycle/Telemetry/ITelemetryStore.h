#pragma once

#include "Bms/Models/BatteryTelemetry.h"
#include "Telemetry/EspHealth.h"
#include "Telemetry/GatewayStatus.h"

namespace wattcycle::telemetry {

class ITelemetryStore {
 public:
  virtual ~ITelemetryStore() = default;

  virtual void updateBattery(const bms::BatteryTelemetry& telemetry) = 0;
  virtual void updateProduct(const bms::ProductInfo& product) = 0;
  virtual void updateWarnings(const bms::WarningFlags& warnings) = 0;
  virtual void setBleState(bool connected, const char* address, const char* error) = 0;
  virtual void setEspNowState(bool ready, uint8_t channel, const char* peerMac,
                              const char* error) = 0;
  virtual void setTelemetryFresh(bool fresh) = 0;
  virtual void updateEspHealth(const EspHealth& health) = 0;

  virtual bms::BatteryTelemetry battery() const = 0;
  virtual bms::ProductInfo product() const = 0;
  virtual bms::WarningFlags warnings() const = 0;
  virtual GatewayStatus status() const = 0;
  virtual EspHealth espHealth() const = 0;
};

}  // namespace wattcycle::telemetry
