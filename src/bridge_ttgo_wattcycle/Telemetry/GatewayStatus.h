#pragma once

#include <cstdint>

namespace wattcycle::telemetry {

struct GatewayStatus {
  bool bleConnected = false;
  bool telemetryFresh = false;
  bool espNowReady = false;
  uint8_t espNowChannel = 0;
  char bleAddress[18] = {};
  char espNowPeerMac[18] = {};
  char lastError[64] = {};
};

}  // namespace wattcycle::telemetry
