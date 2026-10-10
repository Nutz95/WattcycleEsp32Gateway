#pragma once

#include "EcoFlow/Ble/EcoFlowBleSnapshot.h"
#include "EcoFlow/Models/PowerStationTelemetry.h"

#include <cstddef>
#include <cstdint>

namespace ecoflow::ble {

/// Read-only BLE transport for EcoFlow DELTA 3 (V3). No control commands.
class IEcoFlowBleClient {
 public:
  virtual ~IEcoFlowBleClient() = default;

  /// Initialize BLE target and start read-only session (no control writes).
  virtual bool begin(const char* bleAddress, const char* serial, const char* userId) = 0;
  /// Periodic connect/auth/telemetry pump; call from the dedicated BLE task.
  virtual void loop() = 0;
  /// True when BLE link is established.
  virtual bool isConnected() const = 0;
  /// True when EcoFlow V3 auth handshake succeeded.
  virtual bool isAuthenticated() const = 0;
  /// Diagnostic error string when connect or auth fails.
  virtual const char* lastError() const = 0;

  /// Number of raw notify chunks received (debug / bring-up).
  virtual uint32_t notifyCount() const = 0;
  /// Count of decoded telemetry updates exposed via telemetry().
  virtual uint32_t telemetryCount() const = 0;
  /// Latest read-only power-station telemetry (by value; safe across tasks).
  virtual models::PowerStationTelemetry telemetry() const = 0;
  /// Atomic view for ESP-NOW / heartbeat on the Arduino loop core.
  virtual EcoFlowBleSnapshot snapshot() const = 0;
};

}  // namespace ecoflow::ble
