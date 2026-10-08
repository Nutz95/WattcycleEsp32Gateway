#pragma once

#include "Config/AppConfig.h"
#include "Meter/Spp/ISppMeterClient.h"
#include "Telemetry/ITelemetryStore.h"

namespace xt369p::telemetry {

/// Owns SPP connect / reconnect / decode on the radio core.
class TelemetryPoller {
 public:
  TelemetryPoller(meter::ISppMeterClient& client, ITelemetryStore& store,
                  const config::AppConfig& config);

  void begin();
  /// Start Classic BT after Wi-Fi driver init (avoids ESP_ERR_NO_MEM on wifi task).
  void enableRadio();
  void loop();

 private:
  void ensureConnected(uint32_t nowMs);
  void drainCommands();
  const char* targetLabel() const;

  meter::ISppMeterClient& client_;
  ITelemetryStore& store_;
  config::AppConfig config_;
  uint32_t lastConnectAttemptMs_ = 0;
  bool started_ = false;
  char targetLabel_[32] = {};
};

}  // namespace xt369p::telemetry
