#include "Display/P4FlowBuilder.h"

#include <cmath>

namespace wattcycle::display {
namespace {

constexpr float kFlowEpsW = 3.0f;
constexpr float kSolarEpsW = 1.0f;

int roundWatts(float value) {
  return static_cast<int>(std::floor(value + 0.5f));
}

float portBalance(const telemetry::EcoFlowBridgeTelemetry& eco, bool packValid, float packW) {
  float packIn = 0.0f;
  float packOut = 0.0f;
  if (packValid && std::fabs(packW) > kFlowEpsW) {
    if (packW < 0.0f) {
      packIn = std::fabs(packW);
    } else {
      packOut = packW;
    }
  }
  const float outs = static_cast<float>(eco.acOutputW) + static_cast<float>(eco.dcOutputW) +
                     static_cast<float>(eco.usbOutputW);
  return static_cast<float>(eco.acInputW) + static_cast<float>(eco.solarInputW) + packIn - outs -
         packOut;
}

void fillStationUse(P4OverviewFlow& flow, const telemetry::EcoFlowBridgeTelemetry& eco,
                    bool packValid, float packW) {
  if (!eco.meterValid) {
    return;
  }
  flow.ecoValid = true;
  flow.ecoSoc = eco.socPercent;
  const float batteryW = static_cast<float>(eco.bmsPowerW);
  if (std::fabs(batteryW) > kFlowEpsW) {
    flow.showEcoBattery = true;
    flow.ecoBatteryW = roundWatts(batteryW);
  }
  const float balance = portBalance(eco, packValid, packW);
  if (std::fabs(balance) > kFlowEpsW) {
    flow.showBalance = true;
    flow.balanceW = roundWatts(balance);
  }
  if (std::fabs(batteryW) <= kFlowEpsW) {
    return;
  }
  const float self = balance - batteryW;
  if (self <= kFlowEpsW) {
    return;
  }
  flow.showSelf = true;
  flow.selfW = roundWatts(self);
}

void fillPorts(P4OverviewFlow& flow, const telemetry::EcoFlowBridgeTelemetry& eco) {
  if (!eco.meterValid) {
    return;
  }
  flow.acInW = eco.acInputW;
  flow.acInActive = static_cast<float>(eco.acInputW) > kFlowEpsW;
  flow.acOutW = eco.acOutputW;
  flow.acOutOn = eco.acOutputOn;
  flow.dcOutW = eco.dcOutputW;
  flow.dcOutOn = eco.dcOutputOn;
  flow.usbOutW = eco.usbOutputW;
  flow.usbOutOn = eco.usbOutputOn;
}

}  // namespace

P4OverviewFlow buildP4Overview(const telemetry::ITelemetryStore& store) {
  const auto battery = store.battery();
  const auto eco = store.ecoflow();
  const auto solar = store.solar();
  const auto status = store.status();

  P4OverviewFlow flow;
  if (status.telemetryFresh && status.bleConnected) {
    flow.packLink = P4Link::Up;
  }
  if (eco.linkFresh && eco.bleConnected) {
    flow.ecoLink = P4Link::Up;
  }
  if (solar.linkFresh && solar.sppConnected) {
    flow.solarLink = P4Link::Up;
  }

  flow.packValid = battery.valid;
  if (battery.valid) {
    flow.packSoc = battery.stateOfChargePercent;
    if (std::fabs(battery.powerWatts) > kFlowEpsW) {
      flow.packWatts = roundWatts(battery.powerWatts);
      flow.packEdge.active = true;
      flow.packEdge.reverse = battery.powerWatts < 0.0f;
    }
  }

  fillStationUse(flow, eco, battery.valid, battery.powerWatts);
  fillPorts(flow, eco);

  if (solar.meterValid) {
    flow.solarW = roundWatts(solar.powerW);
  }
  flow.solarActive = solar.linkFresh && solar.meterValid && solar.powerW > kSolarEpsW;
  return flow;
}

}  // namespace wattcycle::display
