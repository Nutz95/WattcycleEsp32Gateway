#pragma once

#include "Esp/EspHealthSnapshot.h"

#include <cstdint>

namespace ecoflow::esp_sys {

/// Samples FreeRTOS idle hooks + chip sensors for ESP-NOW health trailer.
class EspHealthSampler {
 public:
  /// Register idle hooks (best-effort CPU %).
  void begin();
  /// Refresh snapshot when sample interval elapsed; returns true if updated.
  bool sample();
  /// Latest sample (zeros until first successful sample).
  EspHealthSnapshot snapshot() const;

 private:
  EspHealthSnapshot health_{};
  uint32_t lastSampleMs_ = 0;
  uint32_t prevIdle0_ = 0;
  uint32_t prevIdle1_ = 0;
  uint32_t maxIdle0PerSec_ = 1;
  uint32_t maxIdle1PerSec_ = 1;
};

}  // namespace ecoflow::esp_sys
