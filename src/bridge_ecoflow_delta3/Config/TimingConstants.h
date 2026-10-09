#pragma once

#include <cstdint>

namespace ecoflow::config {

struct TimingConstants {
  static constexpr uint32_t kHeartbeatMs = 3000;
  static constexpr uint32_t kAppLoopDelayMs = 50;
};

}  // namespace ecoflow::config
