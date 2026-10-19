#include "Telemetry/TelemetryPoller.h"

#include "Config/TimingConstants.h"

#include <Arduino.h>

namespace wattcycle::telemetry {

TelemetryPoller::TelemetryPoller(bms::IBmsBleClient& bleClient, ITelemetryStore& store,
                                 const config::AppConfig& appConfig)
    : bleClient_(bleClient), store_(store), appConfig_(appConfig) {}

void TelemetryPoller::begin() {
  lastPollMs_ = 0;
  connectAttempts_ = 0;
  nextConnectAllowedMs_ = 0;
  frameHeadDetected_ = false;
  productInfoLoaded_ = false;
  store_.setBleConnectProgress(0, 0);
}

void TelemetryPoller::failAndBackoff(uint32_t nowMs, const char* error) {
  nextConnectAllowedMs_ = nowMs + config::TimingConstants::kBleReconnectIntervalMs;
  store_.setBleState(false, appConfig_.bmsBleAddress, error);
  store_.setBleConnectProgress(connectAttempts_, nextConnectAllowedMs_);
  store_.setTelemetryFresh(false);
}

bool TelemetryPoller::ensureConnected(uint32_t nowMs) {
  if (bleClient_.isConnected()) {
    return true;
  }

  if (!config::AppConfigFactory::hasBmsAddress(appConfig_)) {
    store_.setBleState(false, appConfig_.bmsBleAddress, "BMS_BLE_ADDRESS missing");
    store_.setBleConnectProgress(connectAttempts_, 0);
    store_.setTelemetryFresh(false);
    return false;
  }

  if (nextConnectAllowedMs_ != 0 && nowMs < nextConnectAllowedMs_) {
    store_.setBleConnectProgress(connectAttempts_, nextConnectAllowedMs_);
    return false;
  }

  ++connectAttempts_;
  store_.setBleConnectProgress(connectAttempts_, 0);
  store_.setBleState(false, appConfig_.bmsBleAddress, "BLE connecting...");
  store_.setTelemetryFresh(false);

  if (!bleClient_.connect(appConfig_.bmsBleAddress, appConfig_.bleConnectTimeoutMs)) {
    failAndBackoff(nowMs, "BLE connect failed");
    return false;
  }

  frameHeadDetected_ = bleClient_.detectFrameHead();
  if (!frameHeadDetected_) {
    bleClient_.disconnect();
    failAndBackoff(nowMs, "Frame head detect failed");
    return false;
  }

  nextConnectAllowedMs_ = 0;
  store_.setBleState(true, appConfig_.bmsBleAddress, "");
  store_.setBleConnectProgress(connectAttempts_, 0);
  return true;
}

void TelemetryPoller::loop(uint32_t nowMs) {
  if (!ensureConnected(nowMs)) {
    return;
  }
  if (lastPollMs_ != 0 && (nowMs - lastPollMs_) < appConfig_.bmsPollIntervalMs) {
    return;
  }
  pollOnce(nowMs);
  lastPollMs_ = nowMs;
}

void TelemetryPoller::pollOnce(uint32_t nowMs) {
  bms::BatteryTelemetry battery;
  if (bleClient_.readAnalogQuantity(battery)) {
    battery.updatedAtMs = nowMs;
    store_.updateBattery(battery);
    store_.setTelemetryFresh(true);
  } else {
    store_.setBleState(bleClient_.isConnected(), appConfig_.bmsBleAddress,
                       "Analog read failed");
  }

  if (!productInfoLoaded_) {
    bms::ProductInfo product;
    if (bleClient_.readProductInfo(product)) {
      store_.updateProduct(product);
      productInfoLoaded_ = true;
    }
  }

  bms::WarningFlags warnings;
  if (bleClient_.readWarningFlags(warnings)) {
    store_.updateWarnings(warnings);
  }
}

}  // namespace wattcycle::telemetry
