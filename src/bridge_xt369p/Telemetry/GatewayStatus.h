#pragma once

#include <cstdint>

namespace xt369p::telemetry {

struct GatewayStatus {
  bool sppConnected = false;
  bool telemetryFresh = false;
  bool espNowReady = false;
  uint8_t espNowChannel = 0;
  char sppTarget[32] = {};
  char espNowPeerMac[18] = {};
  char lastError[64] = {};
};

}  // namespace xt369p::telemetry
