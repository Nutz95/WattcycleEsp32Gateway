#pragma once

#include "EspNow/IEspNowTelemetryPublisher.h"

#include <cstdint>

namespace ecoflow::espnow_tx {

class EspNowTelemetryPublisher : public IEspNowTelemetryPublisher {
 public:
  /// Bring up STA radio + ESP-NOW peer (no router join).
  bool begin(const char* wifiSsid, const char* peerMac, uint8_t fixedChannel = 0,
             const char* pmk = "") override;
  /// True after STA channel lock and ESP-NOW peer are configured.
  bool isReady() const override;
  /// Wi-Fi channel used for ESP-NOW (from SSID scan or fixed).
  uint8_t channel() const override;
  /// Encode EcoFlow telemetry + health into ESP-NOW frame and send to hub peer.
  void publish(const models::PowerStationTelemetry& telemetry, bool bleConnected,
               uint32_t telemetryCount, const char* lastError,
               const esp_sys::EspHealthSnapshot& health) override;
  /// Last ESP-NOW init or send failure message.
  const char* lastError() const override;

 private:
  uint8_t resolveChannel(const char* wifiSsid, uint8_t fixedChannel) const;
  bool initRadio(const char* wifiSsid, uint8_t fixedChannel);
  bool addPeer(const char* pmk);

  bool ready_ = false;
  bool encrypt_ = false;
  uint8_t channel_ = 0;
  uint8_t peerMac_[6] = {};
  uint32_t seq_ = 0;
  char lastError_[48] = {};
};

}  // namespace ecoflow::espnow_tx
