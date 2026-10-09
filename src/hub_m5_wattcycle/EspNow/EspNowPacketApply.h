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

}  // namespace wattcycle::espnow_rx
