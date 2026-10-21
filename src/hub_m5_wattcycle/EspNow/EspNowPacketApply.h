#pragma once

#include "EspNow/EcoFlowEspNowProtocol.h"
#include "EspNow/WattcycleEspNowProtocol.h"
#include "EspNow/Xt369pEspNowProtocol.h"
#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::espnow_rx {

void applySolarPacket(telemetry::ITelemetryStore& store, bool encrypt,
                      const xt369p_bridge::EspNowPacketV1& packet);
void applyEcoFlowPacket(telemetry::ITelemetryStore& store, bool encrypt,
                        const ecoflow_bridge::EspNowPacketV1& packet);
void applyBmsTelemetryPacket(telemetry::ITelemetryStore& store,
                             const wattcycle_bridge::EspNowTelemetryPacketV1& packet);
void applyBmsProductPacket(telemetry::ITelemetryStore& store,
                           const wattcycle_bridge::EspNowProductPacketV1& packet);

/// Copy one ESP-NOW frame into the matching inbox slot. Runs on the radio callback.
void ingestEspNowFrame(const uint8_t* mac, const uint8_t* data, int len, uint8_t xtPeerMac[6],
                       bool& hasXtPeer, volatile bool& pendingSolar, volatile bool& pendingEcoFlow,
                       volatile bool& pendingBmsTelemetry, volatile bool& pendingBmsProduct,
                       xt369p_bridge::EspNowPacketV1& solarPacket,
                       ecoflow_bridge::EspNowPacketV1& ecoPacket,
                       wattcycle_bridge::EspNowTelemetryPacketV1& bmsPacket,
                       wattcycle_bridge::EspNowProductPacketV1& productPacket);

}  // namespace wattcycle::espnow_rx
