#pragma once

#include "Bms/Models/BatteryTelemetry.h"
#include "Telemetry/EcoFlowBridgeTelemetry.h"
#include "Telemetry/EspHealth.h"
#include "Telemetry/GatewayStatus.h"
#include "Telemetry/SolarBridgeTelemetry.h"

namespace wattcycle::telemetry {

/// Latest snapshot a display or the web codec can read. Writers live on ITelemetryStore.
class ITelemetryReader {
 public:
  virtual ~ITelemetryReader() = default;

  /// Latest BMS pack, cells, and temperatures.
  virtual bms::BatteryTelemetry battery() const = 0;
  /// BMS product text. Guests never receive the serial.
  virtual bms::ProductInfo product() const = 0;
  /// Active BMS warnings.
  virtual bms::WarningFlags warnings() const = 0;
  /// Latest XT369P bridge snapshot.
  virtual SolarBridgeTelemetry solar() const = 0;
  /// Latest EcoFlow bridge snapshot.
  virtual EcoFlowBridgeTelemetry ecoflow() const = 0;
  /// Hub link flags. Addresses stay empty for a guest.
  virtual GatewayStatus status() const = 0;
  /// Hub ESP health.
  virtual EspHealth espHealth() const = 0;
  /// BMS bridge ESP health.
  virtual EspHealth bmsBridgeHealth() const = 0;
  /// True when the BMS bridge health fields were filled.
  virtual bool bmsBridgeHealthValid() const = 0;
};

}  // namespace wattcycle::telemetry
