#pragma once

#include <cstdint>

namespace wattcycle::config {

struct TimingConstants {
  static constexpr uint32_t kDisplayRefreshMs = 500;
  static constexpr uint32_t kDisplaySleepMs = 60000;
  /// Re-write panel brightness while awake (recovers after brownout glitches).
  static constexpr uint32_t kDisplayBacklightRecoverMs = 15000;
  static constexpr uint32_t kWifiStatusRefreshMs = 2000;
  /// Delay after boot before first ESP-NOW RX bring-up (lets STA/scan settle).
  static constexpr uint32_t kEspNowStartDelayMs = 2000;
  /// Minimum gap between ESP-NOW begin retries when bring-up fails.
  static constexpr uint32_t kEspNowRetryIntervalMs = 15000;
  static constexpr uint32_t kEspHealthSampleMs = 1000;
  /// Mark solar ESP-NOW link stale when no fresh packet arrives.
  static constexpr uint32_t kSolarLinkStaleMs = 5000;
  /// EcoFlow BLE reconnect can pause publishes; keep Overview calm.
  static constexpr uint32_t kEcoFlowLinkStaleMs = 12000;
  /// Mark BMS ESP-NOW link stale (clears telemetryFresh + bleConnected latch).
  static constexpr uint32_t kBmsTelemetryStaleMs = 8000;
  static constexpr uint32_t kAppLoopDelayMs = 10;
  static constexpr uint32_t kButtonDebounceMs = 220;
  static constexpr uint32_t kBleTaskStackWords = 8192;
  static constexpr uint32_t kBleTaskPriority = 1;
  static constexpr uint32_t kDisplayTaskStackWords = 4096;
  static constexpr uint32_t kDisplayTaskPriority = 1;
  /// ESP32 PRO_CPU — Wi-Fi / BT radio affinity.
  static constexpr int kRadioCoreId = 0;
  /// ESP32 APP_CPU — Arduino loop / web / display.
  static constexpr int kAppCoreId = 1;
};

}  // namespace wattcycle::config
