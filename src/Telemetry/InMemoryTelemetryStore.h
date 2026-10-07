#pragma once

#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::telemetry {

class InMemoryTelemetryStore : public ITelemetryStore {
 public:
  void updateBattery(const bms::BatteryTelemetry& telemetry) override;
  void updateProduct(const bms::ProductInfo& product) override;
  void updateWarnings(const bms::WarningFlags& warnings) override;
  void setWifiState(bool connected, const char* ipAddress) override;
  void setBleState(bool connected, const char* address, const char* error) override;
  void setTelemetryFresh(bool fresh) override;

  bms::BatteryTelemetry battery() const override;
  bms::ProductInfo product() const override;
  bms::WarningFlags warnings() const override;
  GatewayStatus status() const override;

 private:
  bms::BatteryTelemetry battery_{};
  bms::ProductInfo product_{};
  bms::WarningFlags warnings_{};
  GatewayStatus status_{};
};

}  // namespace wattcycle::telemetry
