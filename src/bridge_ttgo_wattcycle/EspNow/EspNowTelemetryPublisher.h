#pragma once

#include "EspNow/IEspNowTelemetryPublisher.h"
#include "EspNow/WattcycleEspNowProtocol.h"

#include <cstdint>

namespace wattcycle::espnow_tx {

class EspNowTelemetryPublisher : public IEspNowTelemetryPublisher {
 public:
  bool begin(const char* wifiSsid, const char* peerMac, uint8_t fixedChannel = 0,
             const char* pmk = "") override;
  bool isReady() const override;
  uint8_t channel() const override;
  void publish(const telemetry::ITelemetryStore& store) override;
  const char* lastError() const override;

 private:
  uint8_t resolveChannel(const char* wifiSsid, uint8_t fixedChannel) const;
  void publishProduct(const telemetry::ITelemetryStore& store);

  bool ready_ = false;
  bool encrypt_ = false;
  uint8_t channel_ = 0;
  uint8_t peerMac_[6] = {};
  uint32_t seq_ = 0;
  uint32_t productSeq_ = 0;
  uint32_t lastProductPublishMs_ = 0;
  char lastError_[48] = {};
};

}  // namespace wattcycle::espnow_tx
