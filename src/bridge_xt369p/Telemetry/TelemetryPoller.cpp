#include "Telemetry/TelemetryPoller.h"

#include "Util/MacAddress.h"
#include "Util/SafeCopy.h"

#include <Arduino.h>

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
    addressFailures_ = 0;
    const char* peer = client_.peerAddress();
    store_.setSppState(true, (peer && peer[0]) ? peer : targetLabel_, "");
    return;
  }

  if (lastConnectAttemptMs_ != 0 &&
      (nowMs - lastConnectAttemptMs_) < config_.sppReconnectIntervalMs) {
    return;
  }
  lastConnectAttemptMs_ = nowMs;
  store_.setSppState(false, targetLabel_, "reconnecting");

  bool ok = false;
  const bool haveConfig = addressLooksUsable(config_.xt369pBtAddress);
  const char* remembered = client_.peerAddress();
  const bool haveRemembered = addressLooksUsable(remembered);
  if (haveConfig) {
    ok = client_.connectAddress(config_.xt369pBtAddress);
  } else if (haveRemembered) {
    ok = client_.connectAddress(remembered);
  }
  // SDP already blocks ~10s. A name inquiry on every miss adds another scan and
  // can brown out a thin USB hub, so the dongle looks stuck on reconnect.
  const bool needScan = !haveConfig && !haveRemembered;
  if (!ok && (needScan || addressFailures_ >= 2)) {
    ok = client_.discoverAndConnect(config_.sppDeviceName, 8000);
    addressFailures_ = 0;
  } else if (!ok) {
    addressFailures_++;
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
  const uint32_t nowMs = millis();
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
