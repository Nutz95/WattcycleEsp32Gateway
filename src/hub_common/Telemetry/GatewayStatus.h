#pragma once

#include <cstdint>

namespace wattcycle::telemetry {

struct GatewayStatus {
  bool wifiConnected = false;
  bool bleConnected = false;
  bool telemetryFresh = false;
  bool ntpSynced = false;
  uint16_t webPort = 0;
  char wifiIp[16] = {};
  char bleAddress[18] = {};
  char lastError[64] = {};
  /// Local wall clock for M5 tiles (empty until NTP). "DD/MM/YYYY" + "HH:MM".
  char localDate[11] = {};
  char localTime[6] = {};
};

}  // namespace wattcycle::telemetry
