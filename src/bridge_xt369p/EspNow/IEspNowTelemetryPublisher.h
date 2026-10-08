#pragma once

#include "Telemetry/ITelemetryStore.h"

#include <cstdint>

namespace xt369p::espnow_tx {

class IEspNowTelemetryPublisher {
 public:
  virtual ~IEspNowTelemetryPublisher() = default;

  /// Bring up Wi-Fi radio driver + ESP-NOW peer (no router join / no web).
  /// wifiSsid is only used to discover the AP channel used by the Wattcycle hub.
  /// pmk: optional 16+ char shared key (PMK=LMK). Empty = plaintext ESP-NOW.
  /// store receives downlink meter reset commands from the Wattcycle hub.
  virtual bool begin(const char* wifiSsid, const char* peerMac,
                     telemetry::ITelemetryStore& store, uint8_t fixedChannel = 0,
                     const char* pmk = "") = 0;
  virtual bool isReady() const = 0;
  virtual uint8_t channel() const = 0;
  virtual void publish(const telemetry::ITelemetryStore& store) = 0;
  virtual const char* lastError() const = 0;
};

}  // namespace xt369p::espnow_tx
