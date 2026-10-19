#pragma once

#include <cstdint>

namespace wattcycle::telemetry {

struct GatewayStatus {
  bool bleConnected = false;
  bool telemetryFresh = false;
  bool espNowReady = false;
  uint8_t espNowChannel = 0;
  /// Completed BLE connect attempts since boot (increments on each try).
  uint32_t bleConnectAttempts = 0;
  /// millis() when the next connect may start; 0 = idle/connected or attempt in flight.
  uint32_t bleNextRetryAtMs = 0;
  char bleAddress[18] = {};
  char espNowPeerMac[18] = {};
  char lastError[64] = {};
};

}  // namespace wattcycle::telemetry
