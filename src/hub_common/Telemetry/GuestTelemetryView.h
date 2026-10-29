#pragma once

#include "Auth/IGuestAccess.h"
#include "Telemetry/ITelemetryReader.h"

namespace wattcycle::telemetry {

/// Read-only snapshot that keeps only the guest tabs in `mask` and drops addresses.
class GuestTelemetryView : public ITelemetryReader {
 public:
  GuestTelemetryView(const ITelemetryReader& inner, uint32_t mask);

  /// Pack fields allowed by overview / dashboard / wattcycle / cells / temps tabs.
  bms::BatteryTelemetry battery() const override;
  /// Product text without serial when the wattcycle tab is allowed.
  bms::ProductInfo product() const override;
  /// Warnings when the alerts tab is allowed.
  bms::WarningFlags warnings() const override;
  /// Solar snapshot (brief for overview, full for the solar tab).
  SolarBridgeTelemetry solar() const override;
  /// EcoFlow snapshot (brief for overview, full for the ecoflow tab).
  EcoFlowBridgeTelemetry ecoflow() const override;
  /// Link flags without STA IP / MACs / web port for guests.
  GatewayStatus status() const override;
  /// Hub ESP health when the esp tab is allowed.
  EspHealth espHealth() const override;
  /// BMS bridge health when the esp tab is allowed.
  EspHealth bmsBridgeHealth() const override;
  /// True when bridge health was filled and the esp tab is allowed.
  bool bmsBridgeHealthValid() const override;

 private:
  bool allows(uint32_t bit) const;

  const ITelemetryReader& inner_;
  uint32_t mask_;
};

}  // namespace wattcycle::telemetry
