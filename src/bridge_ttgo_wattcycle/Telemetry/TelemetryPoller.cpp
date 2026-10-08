#include "Telemetry/TelemetryPoller.h"

#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#else
static uint32_t millis() {
  return 0;
}
#endif

namespace wattcycle::telemetry {

TelemetryPoller::TelemetryPoller(bms::IBmsBleClient& bleClient, ITelemetryStore& store,
                                 const config::AppConfig& appConfig)
    : bleClient_(bleClient), store_(store), appConfig_(appConfig) {}

void TelemetryPoller::begin() {
  lastPollMs_ = 0;
  frameHeadDetected_ = false;
  productInfoLoaded_ = false;
}

bool TelemetryPoller::ensureConnected(uint32_t /*nowMs*/) {
  if (bleClient_.isConnected()) {
    return true;
  }

  if (!config::AppConfigFactory::hasBmsAddress(appConfig_)) {
    store_.setBleState(false, appConfig_.bmsBleAddress, "BMS_BLE_ADDRESS missing");
    store_.setTelemetryFresh(false);
    return false;
  }

  const bool connected =
      bleClient_.connect(appConfig_.bmsBleAddress, appConfig_.bleConnectTimeoutMs);
  if (!connected) {
    store_.setBleState(false, appConfig_.bmsBleAddress, "BLE connect failed");
    store_.setTelemetryFresh(false);
    return false;
  }

  frameHeadDetected_ = bleClient_.detectFrameHead();
  if (!frameHeadDetected_) {
    bleClient_.disconnect();
    store_.setBleState(false, appConfig_.bmsBleAddress, "Frame head detect failed");
    store_.setTelemetryFresh(false);
    return false;
  }

  store_.setBleState(true, appConfig_.bmsBleAddress, "");
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
