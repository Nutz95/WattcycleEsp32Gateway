#pragma once
#ifndef P4_STA_RADIO_H
#define P4_STA_RADIO_H

#include "Display/P4PanelBoard.h"
#include "EspNow/EcoFlowEspNowProtocol.h"
#include "EspNow/WattcycleEspNowProtocol.h"
#include "EspNow/Xt369pEspNowProtocol.h"
#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::panel {

/// Wi-Fi STA plus ESP-NOW frames forwarded by the C6. Fills the board from the store.
class P4StaRadio {
 public:
  /// Start the STA and peer MAC slots. Packets land in `store`.
  void begin(display::P4PanelBoard& board, telemetry::ITelemetryStore& store);
  /// Returns true when the board should be painted again.
  bool loop(display::P4PanelBoard& board);
  /// Copy one forwarded ESP-NOW frame into the inbox. Runs on the hosted callback.
  void onFrame(const uint8_t* mac, const uint8_t* data, int len);
  /// Latest C6 free heap in KB. -1 until the coprocessor answers.
  int coprocessorHeapKb() const;

 private:
  bool startEspNow();
  bool applyPending();
  bool expireLinks();
  bool rememberStaIp(display::P4PanelBoard& board);
  /// One-shot coprocessor heap query. The hosted firmware has no CPU or temperature report.
  void pollC6Heap();
  /// ESP-NOW and the C6 update. Runs off the display task so the panel keeps flushing.
  void serviceRadio();
  static void serviceTask(void* arg);

  telemetry::ITelemetryStore* store_ = nullptr;
  bool joinStarted_ = false;
  bool haveMac_ = false;
  bool toldJoinWithoutMac_ = false;
  bool espNowReady_ = false;
  bool c6UpdateStarted_ = false;
  bool encrypt_ = false;
  volatile bool wifiUpFlag_ = false;
  volatile int c6HeapKb_ = -1;
  volatile uint32_t revision_ = 0;
  uint32_t nextC6HeapMs_ = 0;
  uint32_t paintedRevision_ = 0;
  uint32_t joinStartedMs_ = 0;
  uint32_t nextEspNowTryMs_ = 0;
  uint8_t xtPeerMac_[6] = {};
  bool hasXtPeer_ = false;
  volatile bool pendingSolar_ = false;
  volatile bool pendingEco_ = false;
  volatile bool pendingBms_ = false;
  volatile bool pendingProduct_ = false;
  xt369p_bridge::EspNowPacketV1 solarPacket_{};
  ecoflow_bridge::EspNowPacketV1 ecoPacket_{};
  wattcycle_bridge::EspNowTelemetryPacketV1 bmsPacket_{};
  wattcycle_bridge::EspNowProductPacketV1 productPacket_{};
};

}  // namespace wattcycle::panel

#endif
