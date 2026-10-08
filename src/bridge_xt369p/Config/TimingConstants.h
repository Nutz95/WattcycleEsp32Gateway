#pragma once

#include <cstdint>

namespace xt369p::config {

struct TimingConstants {
  static constexpr uint32_t kDisplayRefreshMs = 500;
  static constexpr uint32_t kDisplaySleepMs = 60000;
  static constexpr uint32_t kEspNowPublishMs = 500;
  static constexpr uint32_t kEspHealthSampleMs = 1000;
  static constexpr uint32_t kAppLoopDelayMs = 10;
  static constexpr uint32_t kButtonDebounceMs = 220;
  static constexpr uint32_t kSppTaskStackWords = 5120;
  static constexpr uint32_t kSppTaskPriority = 1;
  static constexpr uint32_t kDisplayTaskStackWords = 3072;
  static constexpr uint32_t kDisplayTaskPriority = 1;
  /// ESP32 PRO_CPU — Wi-Fi / BT radio affinity.
  static constexpr int kRadioCoreId = 0;
  /// ESP32 APP_CPU — Arduino loop / web / display.
  static constexpr int kAppCoreId = 1;
};

}  // namespace xt369p::config
