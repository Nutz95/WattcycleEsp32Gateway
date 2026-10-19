#pragma once

#include "Telemetry/ITelemetryStore.h"

#include <cstdint>

namespace wattcycle::espnow_tx {

/// Publishes BMS telemetry snapshots to the hub over ESP-NOW.
class IEspNowTelemetryPublisher {
 public:
  virtual ~IEspNowTelemetryPublisher() = default;

  /// Join Wi-Fi channel (or fixed channel) and add the hub peer.
  virtual bool begin(const char* wifiSsid, const char* peerMac, uint8_t fixedChannel = 0,
                     const char* pmk = "") = 0;
  /// True after a successful begin.
  virtual bool isReady() const = 0;
  /// Wi-Fi channel used for ESP-NOW.
  virtual uint8_t channel() const = 0;
  /// Send one framed telemetry packet from the store.
  virtual void publish(const telemetry::ITelemetryStore& store) = 0;
  /// Last begin/publish error (empty when OK).
  virtual const char* lastError() const = 0;
};

}  // namespace wattcycle::espnow_tx
