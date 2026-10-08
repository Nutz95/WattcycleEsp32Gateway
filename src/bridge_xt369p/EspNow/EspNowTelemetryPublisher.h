#pragma once

#include "EspNow/IEspNowTelemetryPublisher.h"
#include "EspNow/Xt369pEspNowProtocol.h"

#include <cstdint>

namespace xt369p::espnow_tx {

class EspNowTelemetryPublisher : public IEspNowTelemetryPublisher {
 public:
  bool begin(const char* wifiSsid, const char* peerMac, telemetry::ITelemetryStore& store,
             uint8_t fixedChannel = 0, const char* pmk = "") override;
  bool isReady() const override;
  uint8_t channel() const override;
  void publish(const telemetry::ITelemetryStore& store) override;
  const char* lastError() const override;

 private:
  uint8_t resolveChannel(const char* wifiSsid, uint8_t fixedChannel) const;
  static void onReceiveTrampoline(const uint8_t* mac, const uint8_t* data, int len);
  void onReceive(const uint8_t* mac, const uint8_t* data, int len);

  bool ready_ = false;
  bool encrypt_ = false;
  uint8_t channel_ = 0;
  uint8_t peerMac_[6] = {};
  uint32_t seq_ = 0;
  char lastError_[48] = {};
  telemetry::ITelemetryStore* store_ = nullptr;
  static EspNowTelemetryPublisher* instance_;
};

}  // namespace xt369p::espnow_tx
