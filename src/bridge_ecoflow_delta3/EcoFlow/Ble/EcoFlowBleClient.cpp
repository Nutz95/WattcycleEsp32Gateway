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

bool EcoFlowBleClient::begin(const char* bleAddress, const char* serial, const char* userId) {
  if (bleAddress == nullptr || serial == nullptr || userId == nullptr) {
    setError("null ble config");
    return false;
  }
  if (std::strlen(bleAddress) == 0 || std::strlen(serial) == 0 || std::strlen(userId) == 0) {
    setError("empty ble config");
    return false;
  }

  std::snprintf(bleAddress_, sizeof(bleAddress_), "%s", bleAddress);
  std::snprintf(serial_, sizeof(serial_), "%s", serial);
  std::snprintf(userId_, sizeof(userId_), "%s", userId);
  reconnectDelayMs_ = TimingConstants::kBleReconnectMinMs;
  lastReconnectMs_ = 0;
  failureCount_ = 0;
  paused_ = false;
  haveAddressType_ = false;
  preferredAddressType_ = 0;
  started_ = true;

  auth_.configure(writeTrampoline, this, serial_, userId_);

  NimBLEDevice::init("ecoflow-bridge");
  NimBLEDevice::setPower(9);
  NimBLEDevice::setMTU(TimingConstants::kBlePreferredMtu);
  NimBLEDevice::deleteAllBonds();
  NimBLEDevice::setSecurityAuth(false, false, false);

  Serial.printf("[ecoflow-ble] begin addr=%s serial=%s (NimBLE 2.x, V3 auth)\n", bleAddress_,
                serial_);
  return true;
}

void EcoFlowBleClient::loop() {
  if (!started_ || paused_) {
    return;
  }
  if (connected_) {
    if (!gatt_.isLinkUp()) {
      connected_ = false;
      setError("ble link dropped");
      scheduleReconnectBackoff();
      gatt_.disconnect();
      auth_.reset();
      return;
    }
    auth_.poll();
    if (auth_.state() == EcoFlowBleAuth::State::Failed) {
      setError(auth_.lastError());
      noteFailure(auth_.lastError());
    }
    return;
  }

  const uint32_t now = millis();
  if (lastReconnectMs_ != 0 && (now - lastReconnectMs_) < reconnectDelayMs_) {
    return;
  }
  lastReconnectMs_ = now;

  if (!connectOnce()) {
    noteFailure(lastError_);
  }
}

bool EcoFlowBleClient::isConnected() const {
  return connected_;
}

bool EcoFlowBleClient::isAuthenticated() const {
  return auth_.isAuthenticated();
}

const char* EcoFlowBleClient::lastError() const {
  if (auth_.lastError()[0] != '\0') {
    return auth_.lastError();
  }
  return lastError_;
}

uint32_t EcoFlowBleClient::notifyCount() const {
  return notifyCount_;
}

uint32_t EcoFlowBleClient::telemetryCount() const {
  return auth_.telemetryCount();
}

const models::PowerStationTelemetry& EcoFlowBleClient::telemetry() const {
  return auth_.telemetry();
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
  ++notifyCount_;
  auth_.onNotify(data, length);
}

bool EcoFlowBleClient::connectOnce() {
  Serial.printf("[ecoflow-ble] connect attempt to %s\n", bleAddress_);

  bool ok = gatt_.connectFromScan(bleAddress_, preferredAddressType_, haveAddressType_);
  if (!ok) {
    Serial.println("[ecoflow-ble] scan-path failed — address fallback");
    ok = gatt_.connectByAddress(bleAddress_, haveAddressType_, preferredAddressType_);
  }
  if (!ok) {
    // Mid-handshake crashes leave the peer sticky; recycle the controller.
    Serial.println("[ecoflow-ble] recycling NimBLE stack after connect failure");
    gatt_.disconnect();
    NimBLEDevice::deinit(true);
    delay(TimingConstants::kBleDisconnectSettleMs * 5);
    NimBLEDevice::init("ecoflow-bridge");
    NimBLEDevice::setPower(9);
    NimBLEDevice::setMTU(TimingConstants::kBlePreferredMtu);
    NimBLEDevice::deleteAllBonds();
    NimBLEDevice::setSecurityAuth(false, false, false);
    setError("connect failed");
    return false;
  }
  if (!gatt_.isLinkUp()) {
    setError("dropped before gatt");
    return false;
  }
  if (!gatt_.resolveCharacteristics(notifyTrampoline, this)) {
    setError("gatt uuid lookup failed");
    gatt_.disconnect();
    return false;
  }

  auth_.reset();
  auth_.configure(writeTrampoline, this, serial_, userId_);
  if (!auth_.start()) {
    setError(auth_.lastError());
    gatt_.disconnect();
    return false;
  }

  connected_ = true;
  failureCount_ = 0;
  reconnectDelayMs_ = TimingConstants::kBleReconnectMinMs;
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

void EcoFlowBleClient::noteFailure(const char* message) {
  ++failureCount_;
  if (message != nullptr && message[0] != '\0') {
    Serial.printf("[ecoflow-ble] fail #%u: %s\n", static_cast<unsigned>(failureCount_), message);
  }
  connected_ = false;
  gatt_.disconnect();
  auth_.reset();
  scheduleReconnectBackoff();
  if (failureCount_ >= TimingConstants::kBleMaxFailuresBeforePause) {
    paused_ = true;
    Serial.printf(
        "[ecoflow-ble] paused after %u failures (reboot bridge to retry; stop EcoFlow beeping)\n",
        static_cast<unsigned>(failureCount_));
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
