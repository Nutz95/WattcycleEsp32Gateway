#pragma once

#include "EcoFlow/Ble/EcoFlowBleAuth.h"
#include "EcoFlow/Ble/EcoFlowBleGatt.h"
#include "EcoFlow/Ble/IEcoFlowBleClient.h"

#include <cstddef>
#include <cstdint>
#include <mutex>

namespace ecoflow::ble {

/// NimBLE connect + V3 auth + decrypted notify dump (read-only).
class EcoFlowBleClient : public IEcoFlowBleClient {
 public:
  EcoFlowBleClient();
  ~EcoFlowBleClient() override;

  /// Store targets, init NimBLE, and start connect + auth.
  bool begin(const char* bleAddress, const char* serial, const char* userId) override;
  /// Drive reconnect backoff, auth poll, and link maintenance.
  void loop() override;
  /// True when GATT link is up (may still be authenticating).
  bool isConnected() const override;
  /// True when V3 session auth completed successfully.
  bool isAuthenticated() const override;
  /// Last connect/auth/GATT error for diagnostics (points at internal buffer; prefer snapshot()).
  const char* lastError() const override;
  /// Raw notify chunks received on the EcoFlow characteristic.
  uint32_t notifyCount() const override;
  /// Decoded telemetry updates from auth/display decoder.
  uint32_t telemetryCount() const override;
  /// Latest merged power-station telemetry snapshot (by value).
  models::PowerStationTelemetry telemetry() const override;
  /// Mutex-protected copy for cross-task ESP-NOW / serial heartbeat.
  EcoFlowBleSnapshot snapshot() const override;

  /// Forward notify bytes into auth (NimBLE callback path).
  void onNotify(const uint8_t* data, size_t length);

 private:
  static bool writeTrampoline(void* user, const uint8_t* data, size_t length);
  static void notifyTrampoline(void* owner, const uint8_t* data, size_t length);

  bool connectOnce(std::unique_lock<std::mutex>& lock);
  void configureNimBleHost();
  void noteFailure(const char* message);
  void handleLinkDropped();
  void maybeUnpause(uint32_t nowMs);
  void scheduleReconnectBackoff();
  void resetReconnectLadder();
  void setError(const char* message);
  void refreshPublishSnapshotLocked();

  mutable std::mutex mutex_;
  EcoFlowBleSnapshot publishSnapshot_{};
  char bleAddress_[18] = {};
  char serial_[24] = {};
  char userId_[24] = {};
  char lastError_[96] = {};
  bool connected_ = false;
  bool started_ = false;
  bool paused_ = false;
  bool haveAddressType_ = false;
  uint8_t preferredAddressType_ = 0;
  uint32_t notifyCount_ = 0;
  uint32_t lastReconnectMs_ = 0;
  uint32_t reconnectDelayMs_ = 0;
  uint32_t pausedAtMs_ = 0;
  uint8_t failureCount_ = 0;
  EcoFlowBleGatt gatt_{};
  EcoFlowBleAuth auth_{};
};

}  // namespace ecoflow::ble
