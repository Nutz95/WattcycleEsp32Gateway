#include "Telemetry/TelemetryPoller.h"

#include "Util/MacAddress.h"
#include "Util/SafeCopy.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

namespace xt369p::telemetry {
namespace {

bool addressLooksUsable(const char* text) {
  uint8_t mac[6] = {};
  if (!util::parseMacAddress(text, mac)) {
    return false;
  }
  // Reject Windows-style truncated placeholders (00:00:00:xx:xx:xx).
  return !(mac[0] == 0 && mac[1] == 0 && mac[2] == 0);
}

}  // namespace

TelemetryPoller::TelemetryPoller(meter::ISppMeterClient& client, ITelemetryStore& store,
                                 const config::AppConfig& config)
    : client_(client), store_(store), config_(config) {
  util::copyCString(targetLabel_, sizeof(targetLabel_), targetLabel());
}

const char* TelemetryPoller::targetLabel() const {
  if (addressLooksUsable(config_.xt369pBtAddress)) {
    return config_.xt369pBtAddress;
  }
  return config_.sppDeviceName ? config_.sppDeviceName : "XT369P_SPP";
}

void TelemetryPoller::begin() {
  // Radio stack is started via enableRadio() before Wi-Fi (contiguous heap).
  started_ = false;
  util::copyCString(targetLabel_, sizeof(targetLabel_), targetLabel());
  store_.setSppState(false, targetLabel_, "waiting_radio");
  lastConnectAttemptMs_ = 0;
}

void TelemetryPoller::enableRadio() {
  if (started_) {
    return;
  }
  started_ = client_.beginAsMaster("XT369P_GW");
  util::copyCString(targetLabel_, sizeof(targetLabel_), targetLabel());
  store_.setSppState(false, targetLabel_, started_ ? "scanning" : client_.lastError());
  lastConnectAttemptMs_ = 0;
}

void TelemetryPoller::ensureConnected(uint32_t nowMs) {
  if (!started_) {
    return;
  }
  if (client_.isConnected()) {
    const char* peer = client_.peerAddress();
    store_.setSppState(true, (peer && peer[0]) ? peer : targetLabel_, "");
    return;
  }

  if (lastConnectAttemptMs_ != 0 &&
      (nowMs - lastConnectAttemptMs_) < config_.sppReconnectIntervalMs) {
    return;
  }
  lastConnectAttemptMs_ = nowMs;

  bool ok = false;
  if (addressLooksUsable(config_.xt369pBtAddress)) {
    ok = client_.connectAddress(config_.xt369pBtAddress);
  }
  if (!ok) {
    // Primary path: inquiry scan by advertised name, then connect.
    ok = client_.discoverAndConnect(config_.sppDeviceName, 12000);
  }
  if (!ok) {
    ok = client_.connectName(config_.sppDeviceName);
  }

  const char* peer = client_.peerAddress();
  store_.setSppState(ok, (peer && peer[0]) ? peer : targetLabel_, ok ? "" : client_.lastError());
  if (!ok) {
    store_.setTelemetryFresh(false);
  }
}

void TelemetryPoller::drainCommands() {
  meter::MeterCommand command = meter::MeterCommand::None;
  if (!store_.dequeueMeterCommand(command)) {
    return;
  }
  if (!client_.isConnected()) {
    store_.enqueueMeterCommand(command);
    return;
  }
  if (!client_.sendCommand(command)) {
    store_.setSppState(true, targetLabel_, client_.lastError());
  }
}

void TelemetryPoller::loop() {
#ifndef UNIT_TEST
  const uint32_t nowMs = millis();
#else
  const uint32_t nowMs = 0;
#endif
  if (!started_) {
    return;
  }
  ensureConnected(nowMs);
  if (!client_.isConnected()) {
    return;
  }

  drainCommands();

  meter::WattmeterTelemetry reading;
  if (client_.poll(reading)) {
    store_.updateMeter(reading);
  }
}

}  // namespace xt369p::telemetry
