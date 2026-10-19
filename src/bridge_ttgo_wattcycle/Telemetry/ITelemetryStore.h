#pragma once

#include "Bms/Models/BatteryTelemetry.h"
#include "Telemetry/EspHealth.h"
#include "Telemetry/GatewayStatus.h"

namespace wattcycle::telemetry {

/// In-RAM telemetry + link status shared by BLE poller, ESP-NOW, and UI.
class ITelemetryStore {
 public:
  virtual ~ITelemetryStore() = default;

  /// Replace the latest battery analog reading.
  virtual void updateBattery(const bms::BatteryTelemetry& telemetry) = 0;
  /// Replace product / serial metadata.
  virtual void updateProduct(const bms::ProductInfo& product) = 0;
  /// Replace protection / fault flags.
  virtual void updateWarnings(const bms::WarningFlags& warnings) = 0;
  /// Update BLE link state and last error string.
  virtual void setBleState(bool connected, const char* address, const char* error) = 0;
  /// Publish BLE reconnect progress for the Link UI (attempts + next retry deadline).
  virtual void setBleConnectProgress(uint32_t attempts, uint32_t nextRetryAtMs) = 0;
  /// Update ESP-NOW peer readiness and channel.
  virtual void setEspNowState(bool ready, uint8_t channel, const char* peerMac,
                              const char* error) = 0;
  /// Mark whether the last BMS poll produced fresh data.
  virtual void setTelemetryFresh(bool fresh) = 0;
  /// Replace ESP heap / CPU health sample.
  virtual void updateEspHealth(const EspHealth& health) = 0;

  /// Latest battery reading (may be invalid).
  virtual bms::BatteryTelemetry battery() const = 0;
  /// Latest product metadata.
  virtual bms::ProductInfo product() const = 0;
  /// Latest warning flags.
  virtual bms::WarningFlags warnings() const = 0;
  /// BLE / ESP-NOW link status for UI and publishers.
  virtual GatewayStatus status() const = 0;
  /// Latest ESP health sample.
  virtual EspHealth espHealth() const = 0;
};

}  // namespace wattcycle::telemetry
