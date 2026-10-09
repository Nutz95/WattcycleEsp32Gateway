#pragma once

#include <cstdint>

namespace ecoflow::config {

struct TimingConstants {
  static constexpr uint32_t kHeartbeatMs = 3000;
  static constexpr uint32_t kEspNowPublishMs = 500;
  static constexpr uint32_t kEspHealthSampleMs = 1000;
  static constexpr uint32_t kAppLoopDelayMs = 20;

  // --- BLE reconnect policy (EcoFlow beeps on each connect attempt) ---
  // Keep retries spaced — each attempt makes the DELTA 3 beep.
  static constexpr uint32_t kBleReconnectMinMs = 15000;
  static constexpr uint32_t kBleReconnectMaxMs = 600000;
  static constexpr uint8_t kBleMaxFailuresBeforePause = 5;

  // --- BLE scan (pre-connect address-type resolve) ---
  static constexpr uint16_t kBleScanIntervalSlots = 80;  // 0.625 ms units
  static constexpr uint16_t kBleScanWindowSlots = 40;
  // NimBLE 2.x scan/connect timeouts are milliseconds.
  static constexpr uint32_t kBleScanDurationMs = 4000;

  // --- BLE connect / link params (1.25 ms interval units, 10 ms timeout units) ---
  // Must match EcoFlow's post-connect update (itvl 16..32, timeout 400) *before*
  // connect — NimBLE copies m_pConnParams into the accept path. Do not call
  // setConnectionParams from onConnParamsUpdateRequest (GAP reentrancy).
  static constexpr uint32_t kBleConnectTimeoutMs = 15000;
  static constexpr uint16_t kBleConnIntervalMin = 16;   // 20 ms (NimBLE + EcoFlow default)
  static constexpr uint16_t kBleConnIntervalMax = 32;   // 40 ms
  static constexpr uint16_t kBleConnLatency = 0;
  static constexpr uint16_t kBleConnSupervisionTimeout = 400;  // 4 s
  static constexpr uint16_t kBleConnScanIntervalSlots = 16;
  static constexpr uint16_t kBleConnScanWindowSlots = 16;
  static constexpr uint16_t kBlePreferredMtu = 247;

  static constexpr uint32_t kBleScanStopSettleMs = 400;
  static constexpr uint32_t kBlePostConnectSettleMs = 0;
  static constexpr uint32_t kBleGattRetryDelayMs = 400;
  static constexpr uint8_t kBleGattDiscoverAttempts = 4;
  static constexpr uint32_t kBleDisconnectSettleMs = 200;
  static constexpr uint32_t kBleNotifyHexMaxBytes = 64;
};

}  // namespace ecoflow::config
