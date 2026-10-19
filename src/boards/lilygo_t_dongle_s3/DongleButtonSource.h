#pragma once

#include "boards/common/IButtonSource.h"

namespace boards::lilygo_t_dongle_s3 {

/// Single BOOT button: short = Next, long = Sleep.
class DongleButtonSource : public boards::input::IButtonSource {
 public:
  /// Configure BOOT GPIO pull-up.
  void begin() override;
  /// Poll short/long press edges.
  boards::input::ButtonEvent poll(uint32_t nowMs) override;

 private:
  uint32_t lastEdgeMs_ = 0;
  uint32_t pressStartedMs_ = 0;
  bool wasPressed_ = false;
  bool longFired_ = false;
};

}  // namespace boards::lilygo_t_dongle_s3
