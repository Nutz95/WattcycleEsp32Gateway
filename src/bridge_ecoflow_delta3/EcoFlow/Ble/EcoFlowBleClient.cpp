#include "EcoFlow/Ble/EcoFlowBleClient.h"

#include "Config/TimingConstants.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <cstdio>
#include <cstring>

namespace ecoflow::ble {

using ecoflow::config::TimingConstants;

EcoFlowBleClient::EcoFlowBleClient() = default;

EcoFlowBleClient::~EcoFlowBleClient() {
  gatt_.disconnect();
}

void EcoFlowBleClient::configureNimBleHost() {
  NimBLEDevice::init("ecoflow-bridge");
  // Keep BLE TX modest — dual-radio peaks brownout thin USB rails.
  NimBLEDevice::setPower(3);
  NimBLEDevice::setMTU(TimingConstants::kBlePreferredMtu);
  NimBLEDevice::deleteAllBonds();
  NimBLEDevice::setSecurityAuth(false, false, false);
}

bool EcoFlowBleClient::begin(const char* bleAddress, const char* serial, const char* userId) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (bleAddress == nullptr || serial == nullptr || userId == nullptr) {
    setError("null ble config");
    refreshPublishSnapshotLocked();
    return false;
  }
  if (std::strlen(bleAddress) == 0 || std::strlen(serial) == 0 || std::strlen(userId) == 0) {
    setError("empty ble config");
    refreshPublishSnapshotLocked();
    return false;
  }

  std::snprintf(bleAddress_, sizeof(bleAddress_), "%s", bleAddress);
  std::snprintf(serial_, sizeof(serial_), "%s", serial);
  std::snprintf(userId_, sizeof(userId_), "%s", userId);
  resetReconnectLadder();
  lastReconnectMs_ = 0;
  paused_ = false;
  pausedAtMs_ = 0;
  haveAddressType_ = false;
  preferredAddressType_ = 0;
  started_ = true;

  auth_.configure(writeTrampoline, this, serial_, userId_);
  configureNimBleHost();
  refreshPublishSnapshotLocked();

  Serial.printf("[ecoflow-ble] begin addr=%s serial=%s (NimBLE 2.x, V3 auth)\n", bleAddress_,
                serial_);
  return true;
}

void EcoFlowBleClient::loop() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!started_) {
    return;
  }
  maybeUnpause(millis());
  if (paused_) {
    refreshPublishSnapshotLocked();
    return;
  }
  if (connected_) {
    if (!gatt_.isLinkUp()) {
      handleLinkDropped();
      refreshPublishSnapshotLocked();
      return;
    }
    auth_.poll();
    if (auth_.state() == EcoFlowBleAuth::State::Failed) {
      setError(auth_.lastError());
      noteFailure(auth_.lastError());
    }
    refreshPublishSnapshotLocked();
    return;
  }

  const uint32_t now = millis();
  if (lastReconnectMs_ != 0 && (now - lastReconnectMs_) < reconnectDelayMs_) {
    refreshPublishSnapshotLocked();
    return;
  }
  lastReconnectMs_ = now;

  // Publish last known view before a blocking connect so the Arduino loop
  // can keep ESP-NOW TX alive without racing live BLE state.
  refreshPublishSnapshotLocked();
  if (!connectOnce(lock)) {
    noteFailure(lastError_);
  }
  refreshPublishSnapshotLocked();
}

bool EcoFlowBleClient::isConnected() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return publishSnapshot_.connected;
}

bool EcoFlowBleClient::isAuthenticated() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return publishSnapshot_.authenticated;
}

const char* EcoFlowBleClient::lastError() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return lastError_;
}

uint32_t EcoFlowBleClient::notifyCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return publishSnapshot_.notifyCount;
}

uint32_t EcoFlowBleClient::telemetryCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return publishSnapshot_.telemetryCount;
}

models::PowerStationTelemetry EcoFlowBleClient::telemetry() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return publishSnapshot_.telemetry;
}

EcoFlowBleSnapshot EcoFlowBleClient::snapshot() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return publishSnapshot_;
}

void EcoFlowBleClient::refreshPublishSnapshotLocked() {
  publishSnapshot_.telemetry = auth_.telemetry();
  publishSnapshot_.connected = connected_;
  publishSnapshot_.authenticated = auth_.isAuthenticated();
  publishSnapshot_.notifyCount = notifyCount_;
  publishSnapshot_.telemetryCount = auth_.telemetryCount();
  const char* err = auth_.lastError()[0] != '\0' ? auth_.lastError() : lastError_;
  std::snprintf(publishSnapshot_.lastError, sizeof(publishSnapshot_.lastError), "%s", err);
  std::snprintf(lastError_, sizeof(lastError_), "%s", err);
}

void EcoFlowBleClient::notifyTrampoline(void* owner, const uint8_t* data, size_t length) {
  auto* self = static_cast<EcoFlowBleClient*>(owner);
  if (self != nullptr) {
    self->onNotify(data, length);
  }
}

bool EcoFlowBleClient::writeTrampoline(void* user, const uint8_t* data, size_t length) {
  auto* self = static_cast<EcoFlowBleClient*>(user);
  return self != nullptr && self->gatt_.write(data, length);
}

void EcoFlowBleClient::onNotify(const uint8_t* data, size_t length) {
  std::lock_guard<std::mutex> lock(mutex_);
  ++notifyCount_;
  auth_.onNotify(data, length);
  refreshPublishSnapshotLocked();
}

bool EcoFlowBleClient::connectOnce(std::unique_lock<std::mutex>& lock) {
  // Drop the lock for blocking NimBLE I/O; snapshot stays readable for TX.
  char address[18];
  uint8_t addrType = preferredAddressType_;
  bool haveType = haveAddressType_;
  std::snprintf(address, sizeof(address), "%s", bleAddress_);
  lock.unlock();

  Serial.printf("[ecoflow-ble] connect attempt to %s\n", address);

  bool ok = gatt_.connectFromScan(address, addrType, haveType);
  if (!ok) {
    Serial.println("[ecoflow-ble] scan-path failed — address fallback");
    ok = gatt_.connectByAddress(address, haveType, addrType);
  }
  if (!ok) {
    Serial.println("[ecoflow-ble] recycling NimBLE stack after connect failure");
    gatt_.disconnect();
    NimBLEDevice::deinit(true);
    delay(TimingConstants::kBleDisconnectSettleMs * 5);
    configureNimBleHost();
    lock.lock();
    haveAddressType_ = false;
    setError("connect failed");
    return false;
  }
  if (!gatt_.isLinkUp()) {
    lock.lock();
    setError("dropped before gatt");
    return false;
  }
  if (!gatt_.resolveCharacteristics(notifyTrampoline, this)) {
    gatt_.disconnect();
    lock.lock();
    setError("gatt uuid lookup failed");
    return false;
  }

  lock.lock();
  preferredAddressType_ = addrType;
  haveAddressType_ = haveType;
  auth_.reset();
  auth_.configure(writeTrampoline, this, serial_, userId_);
  if (!auth_.start()) {
    setError(auth_.lastError());
    gatt_.disconnect();
    return false;
  }

  connected_ = true;
  resetReconnectLadder();
  setError("");
  Serial.println("[ecoflow-ble] GATT ready — V3 handshake in progress");
  return true;
}

void EcoFlowBleClient::setError(const char* message) {
  if (message == nullptr) {
    lastError_[0] = '\0';
    return;
  }
  std::snprintf(lastError_, sizeof(lastError_), "%s", message);
}

void EcoFlowBleClient::resetReconnectLadder() {
  failureCount_ = 0;
  reconnectDelayMs_ = TimingConstants::kBleReconnectMinMs;
}

void EcoFlowBleClient::handleLinkDropped() {
  // Phone app / peer disconnect is expected — do not burn pause budget.
  connected_ = false;
  haveAddressType_ = false;
  setError("ble link dropped");
  resetReconnectLadder();
  lastReconnectMs_ = millis();
  gatt_.disconnect();
  auth_.reset();
  Serial.printf("[ecoflow-ble] link dropped — soft reconnect in %lu ms\n",
                static_cast<unsigned long>(reconnectDelayMs_));
}

void EcoFlowBleClient::maybeUnpause(uint32_t nowMs) {
  if (!paused_) {
    return;
  }
  if ((nowMs - pausedAtMs_) < TimingConstants::kBlePauseRecoverMs) {
    return;
  }
  paused_ = false;
  resetReconnectLadder();
  lastReconnectMs_ = 0;
  haveAddressType_ = false;
  Serial.println("[ecoflow-ble] pause expired — resuming reconnect attempts");
}

void EcoFlowBleClient::noteFailure(const char* message) {
  ++failureCount_;
  if (message != nullptr && message[0] != '\0') {
    Serial.printf("[ecoflow-ble] fail #%u: %s\n", static_cast<unsigned>(failureCount_), message);
  }
  connected_ = false;
  haveAddressType_ = false;
  gatt_.disconnect();
  auth_.reset();
  scheduleReconnectBackoff();
  if (failureCount_ >= TimingConstants::kBleMaxFailuresBeforePause) {
    paused_ = true;
    pausedAtMs_ = millis();
    Serial.printf(
        "[ecoflow-ble] paused after %u failures — auto-resume in %lu ms (app may hold BLE)\n",
        static_cast<unsigned>(failureCount_),
        static_cast<unsigned long>(TimingConstants::kBlePauseRecoverMs));
  }
}

void EcoFlowBleClient::scheduleReconnectBackoff() {
  if (reconnectDelayMs_ < TimingConstants::kBleReconnectMaxMs / 2) {
    reconnectDelayMs_ *= 2;
  } else {
    reconnectDelayMs_ = TimingConstants::kBleReconnectMaxMs;
  }
  if (reconnectDelayMs_ < TimingConstants::kBleReconnectMinMs) {
    reconnectDelayMs_ = TimingConstants::kBleReconnectMinMs;
  }
  Serial.printf("[ecoflow-ble] next reconnect in %lu ms\n",
                static_cast<unsigned long>(reconnectDelayMs_));
}

}  // namespace ecoflow::ble
