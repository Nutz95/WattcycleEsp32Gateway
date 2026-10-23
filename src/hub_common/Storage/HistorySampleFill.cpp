#include "Storage/HistorySampleFill.h"

namespace wattcycle::storage {

void fillHistorySample(HistorySample& sample, const bms::BatteryTelemetry& battery,
                       const telemetry::SolarBridgeTelemetry& solar,
                       const telemetry::EcoFlowBridgeTelemetry& eco) {
  sample.socPercent = static_cast<uint8_t>(battery.stateOfChargePercent);
  sample.packVoltageV = battery.moduleVoltage;
  sample.packCurrentA = battery.currentAmps;
  sample.packPowerW = battery.powerWatts;
  sample.packValid = battery.valid;
  sample.packRemainingAh = battery.remainingCapacityAh;
  sample.packMosTempC = battery.mosTemperatureC;
  sample.packPcbTempC = battery.pcbTemperatureC;
  sample.solarVoltageV = solar.voltageV;
  sample.solarCurrentA = solar.currentA;
  sample.solarPowerW = solar.powerW;
  sample.solarValid = solar.meterValid;
  sample.solarEnergyWh = solar.energyWh;
  sample.solarCapacityAh = solar.capacityAh;
  sample.ecoValid = eco.linkFresh && eco.bleConnected && eco.meterValid;
  sample.ecoSocPercent = eco.socPercent;
  sample.ecoAcOutW = eco.acOutputW;
  sample.ecoAcInW = eco.acInputW;
  sample.ecoDcOutW = eco.dcOutputW;
  sample.ecoUsbOutW = eco.usbOutputW;
}

}  // namespace wattcycle::storage
