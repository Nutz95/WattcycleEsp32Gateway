#pragma once

#include "EspNow/IEspNowTelemetryReceiver.h"
#include "EspNow/WattcycleEspNowProtocol.h"
#include "EspNow/Xt369pEspNowProtocol.h"
#include "Telemetry/ITelemetryStore.h"

#include <cstdint>

namespace wattcycle::espnow_rx {

class EspNowTelemetryReceiver : public IEspNowTelemetryReceiver {
 public:
  explicit EspNowTelemetryReceiver(telemetry::ITelemetryStore& store);

  bool begin(const char* bmsBridgeMac = "", const char* xtBridgeMac = "",
             const char* pmk = "", uint8_t channel = 0) override;
  void loop() override;
  bool isReady() const override;
  bool sendMeterCommand(uint8_t command) override;

 private:
  static void onReceiveTrampoline(const uint8_t* mac, const uint8_t* data, int len);
  void onReceive(const uint8_t* mac, const uint8_t* data, int len);
  void applyPending();
  bool ensurePeer(const uint8_t mac[6]);
  void applySolar(const xt369p_bridge::EspNowPacketV1& packet);
  void applyBmsTelemetry(const wattcycle_bridge::EspNowTelemetryPacketV1& packet);
  void applyBmsProduct(const wattcycle_bridge::EspNowProductPacketV1& packet);

  telemetry::ITelemetryStore& store_;
  bool ready_ = false;
  bool encrypt_ = false;
  uint8_t lmk_[16] = {};
  uint8_t xtPeerMac_[6] = {};
  bool hasXtPeer_ = false;
  // Independent pending flags — dual bridges must not overwrite each other.
  volatile bool pendingSolar_ = false;
  volatile bool pendingBmsTelemetry_ = false;
  volatile bool pendingBmsProduct_ = false;
  xt369p_bridge::EspNowPacketV1 solarPacket_{};
  wattcycle_bridge::EspNowTelemetryPacketV1 bmsPacket_{};
  wattcycle_bridge::EspNowProductPacketV1 productPacket_{};
  static EspNowTelemetryReceiver* instance_;
};

}  // namespace wattcycle::espnow_rx
