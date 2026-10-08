#pragma once

#include "EspNow/IEspNowTelemetryReceiver.h"
#include "EspNow/Xt369pEspNowProtocol.h"
#include "Telemetry/ITelemetryStore.h"

#include <cstdint>

namespace wattcycle::espnow_rx {

class EspNowTelemetryReceiver : public IEspNowTelemetryReceiver {
 public:
  explicit EspNowTelemetryReceiver(telemetry::ITelemetryStore& store);

  bool begin(const char* bridgeMac = "", const char* pmk = "") override;
  void loop() override;
  bool isReady() const override;
  bool sendMeterCommand(uint8_t command) override;

 private:
  static void onReceiveTrampoline(const uint8_t* mac, const uint8_t* data, int len);
  void onReceive(const uint8_t* mac, const uint8_t* data, int len);
  void applyPending();
  bool ensurePeer(const uint8_t mac[6]);
  bool parseMac(const char* text, uint8_t out[6]) const;

  telemetry::ITelemetryStore& store_;
  bool ready_ = false;
  bool hasPeer_ = false;
  bool encrypt_ = false;
  uint8_t lmk_[16] = {};
  volatile bool pending_ = false;
  xt369p_bridge::EspNowPacketV1 pendingPacket_{};
  uint8_t pendingMac_[6] = {};
  uint8_t peerMac_[6] = {};
  static EspNowTelemetryReceiver* instance_;
};

}  // namespace wattcycle::espnow_rx
