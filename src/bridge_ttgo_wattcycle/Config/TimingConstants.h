#pragma once

#include <cstdint>

namespace wattcycle::config {

struct TimingConstants {
  static constexpr uint32_t kDisplayRefreshMs = 500;
  static constexpr uint32_t kDisplaySleepMs = 60000;
  static constexpr uint32_t kEspNowPublishMs = 500;
  static constexpr uint32_t kEspHealthSampleMs = 1000;
  static constexpr uint32_t kAppLoopDelayMs = 10;
  static constexpr uint32_t kButtonDebounceMs = 220;
  static constexpr uint32_t kBleTaskStackWords = 6144;
  static constexpr uint32_t kBleTaskPriority = 1;
  static constexpr uint32_t kDisplayTaskStackWords = 3072;
  static constexpr uint32_t kDisplayTaskPriority = 1;
  static constexpr int kRadioCoreId = 0;
  static constexpr int kAppCoreId = 1;
};

}  // namespace wattcycle::config
