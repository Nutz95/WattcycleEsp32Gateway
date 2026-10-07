#pragma once

#include <cstdint>

namespace wattcycle::config {

struct TimingConstants {
  static constexpr uint32_t kDisplayRefreshMs = 2000;
  static constexpr uint32_t kWifiStatusRefreshMs = 1000;
  static constexpr uint32_t kBleTaskStackWords = 8192;
  static constexpr uint32_t kBleTaskPriority = 1;
  /// ESP32 PRO_CPU — Wi-Fi / BT radio affinity.
  static constexpr int kRadioCoreId = 0;
  /// ESP32 APP_CPU — Arduino loop / web / display.
  static constexpr int kAppCoreId = 1;
};

}  // namespace wattcycle::config
