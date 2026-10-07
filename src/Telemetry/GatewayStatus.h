#pragma once

#include <cstdint>

namespace wattcycle::telemetry {

struct GatewayStatus {
  bool wifiConnected = false;
  bool bleConnected = false;
  bool telemetryFresh = false;
  uint16_t webPort = 0;
  char wifiIp[16] = {};
  char bleAddress[18] = {};
  char lastError[64] = {};
};

}  // namespace wattcycle::telemetry
