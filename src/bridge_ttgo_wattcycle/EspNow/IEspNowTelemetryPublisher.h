#pragma once

#include "Telemetry/ITelemetryStore.h"

#include <cstdint>

namespace wattcycle::espnow_tx {

class IEspNowTelemetryPublisher {
 public:
  virtual ~IEspNowTelemetryPublisher() = default;

  virtual bool begin(const char* wifiSsid, const char* peerMac, uint8_t fixedChannel = 0,
                     const char* pmk = "") = 0;
  virtual bool isReady() const = 0;
  virtual uint8_t channel() const = 0;
  virtual void publish(const telemetry::ITelemetryStore& store) = 0;
  virtual const char* lastError() const = 0;
};

}  // namespace wattcycle::espnow_tx
