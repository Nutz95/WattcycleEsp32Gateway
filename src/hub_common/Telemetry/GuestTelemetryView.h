#pragma once

#include "Auth/IGuestAccess.h"
#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::telemetry {

/// Read-only snapshot that keeps only the guest tabs in `mask` and drops addresses.
class GuestTelemetryView : public ITelemetryReader {
 public:
  GuestTelemetryView(const ITelemetryStore& inner, uint32_t mask);

  bms::BatteryTelemetry battery() const override;
  bms::ProductInfo product() const override;
  bms::WarningFlags warnings() const override;
  SolarBridgeTelemetry solar() const override;
  EcoFlowBridgeTelemetry ecoflow() const override;
  GatewayStatus status() const override;
  EspHealth espHealth() const override;
  EspHealth bmsBridgeHealth() const override;
  bool bmsBridgeHealthValid() const override;

 private:
  bool allows(uint32_t bit) const;

  const ITelemetryStore& inner_;
  uint32_t mask_;
};

}  // namespace wattcycle::telemetry
