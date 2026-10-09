#pragma once

#include "EcoFlow/Models/PowerStationTelemetry.h"
#include "Esp/EspHealthSnapshot.h"

#include <cstdint>

namespace ecoflow::espnow_tx {

/// ESP-NOW TX to the M5 hub (read-only telemetry — no downlink commands).
class IEspNowTelemetryPublisher {
 public:
  virtual ~IEspNowTelemetryPublisher() = default;

  /// wifiSsid discovers the AP channel used by the hub (no STA join).
  /// pmk: optional 16+ char shared key. Empty = plaintext ESP-NOW.
  virtual bool begin(const char* wifiSsid, const char* peerMac, uint8_t fixedChannel = 0,
                     const char* pmk = "") = 0;
  /// True after peer + radio init succeeded.
  virtual bool isReady() const = 0;
  /// Locked 2.4 GHz channel (1–13).
  virtual uint8_t channel() const = 0;
  /// Pack and send one EF3P frame (call on a timer from the composition root).
  virtual void publish(const models::PowerStationTelemetry& telemetry, bool bleConnected,
                       uint32_t telemetryCount, const char* lastError,
                       const esp_sys::EspHealthSnapshot& health) = 0;
  /// Last begin/publish setup error (empty when OK).
  virtual const char* lastError() const = 0;
};

}  // namespace ecoflow::espnow_tx
