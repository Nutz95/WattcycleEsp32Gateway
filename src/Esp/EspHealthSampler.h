#pragma once

#include "Telemetry/EspHealth.h"

namespace wattcycle::esp_sys {

/// Samples dual-core idle activity + chip temperature for the ESP tab.
/// Load % uses a high-water idle-tick calibration (avoids a fixed magic rate).
class EspHealthSampler {
 public:
  void begin();
  /// Returns true when a new sample window was closed.
  bool sample();
  telemetry::EspHealth snapshot() const;

 private:
  telemetry::EspHealth health_{};
  uint32_t lastSampleMs_ = 0;
  uint32_t prevIdle0_ = 0;
  uint32_t prevIdle1_ = 0;
  uint32_t maxIdle0PerSec_ = 0;
  uint32_t maxIdle1PerSec_ = 0;
};

}  // namespace wattcycle::esp_sys
