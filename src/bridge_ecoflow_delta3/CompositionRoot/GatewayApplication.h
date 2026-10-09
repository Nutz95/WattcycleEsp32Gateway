#pragma once

#include "Config/AppConfig.h"

namespace ecoflow::composition {

/// Milestone A: USB bring-up heartbeat only (no BLE / ESP-NOW yet).
class GatewayApplication {
 public:
  explicit GatewayApplication(const config::AppConfig& appConfig);

  bool begin();
  void loop();

 private:
  void printHeartbeat(uint32_t nowMs);

  config::AppConfig appConfig_;
  uint32_t lastHeartbeatMs_ = 0;
  uint32_t heartbeatCount_ = 0;
};

}  // namespace ecoflow::composition
