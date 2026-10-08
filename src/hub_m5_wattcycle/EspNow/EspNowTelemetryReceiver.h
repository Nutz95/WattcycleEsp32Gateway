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
             const char* pmk = "") override;
  void loop() override;
  bool isReady() const override;
  bool sendMeterCommand(uint8_t command) override;

 private:
  enum class PendingKind : uint8_t { None, Solar, BmsTelemetry, BmsProduct };

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
  volatile PendingKind pendingKind_ = PendingKind::None;
  xt369p_bridge::EspNowPacketV1 pendingSolar_{};
  wattcycle_bridge::EspNowTelemetryPacketV1 pendingBms_{};
  wattcycle_bridge::EspNowProductPacketV1 pendingProduct_{};
  static EspNowTelemetryReceiver* instance_;
};

}  // namespace wattcycle::espnow_rx
