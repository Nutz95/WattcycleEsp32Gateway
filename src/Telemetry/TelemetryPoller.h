#pragma once

#include <cstdint>

#include "Bms/Ble/IBmsBleClient.h"
#include "Config/AppConfig.h"
#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::telemetry {

/// Periodically polls the BMS and publishes snapshots into the telemetry store.
class TelemetryPoller {
 public:
  TelemetryPoller(bms::IBmsBleClient& bleClient, ITelemetryStore& store,
                  const config::AppConfig& appConfig);

  void begin();
  void loop(uint32_t nowMs);

 private:
  bool ensureConnected(uint32_t nowMs);
  void pollOnce(uint32_t nowMs);

  bms::IBmsBleClient& bleClient_;
  ITelemetryStore& store_;
  config::AppConfig appConfig_;
  uint32_t lastPollMs_ = 0;
  bool frameHeadDetected_ = false;
  bool productInfoLoaded_ = false;
};

}  // namespace wattcycle::telemetry
