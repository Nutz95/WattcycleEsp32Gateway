#include "EspNow/EspNowPacketApply.h"

#include "Telemetry/EcoFlowBridgeTelemetry.h"
#include "Telemetry/SolarBridgeTelemetry.h"
#include "Util/SafeCopy.h"

#include <Arduino.h>

namespace wattcycle::espnow_rx {

void applySolarPacket(telemetry::ITelemetryStore& store, bool encrypt,
                      const xt369p_bridge::EspNowPacketV1& packet) {
  telemetry::SolarBridgeTelemetry solar = {};
  solar.linkFresh = true;
  solar.sppConnected = (packet.flags & xt369p_bridge::kFlagSppConnected) != 0;
  solar.meterValid = (packet.flags & xt369p_bridge::kFlagMeterValid) != 0;
  solar.checksumOk = (packet.flags & xt369p_bridge::kFlagChecksumOk) != 0;
  solar.espNowEncrypted = encrypt || ((packet.flags & xt369p_bridge::kFlagEncrypted) != 0);
  solar.voltageV = packet.voltageCv / 100.0f;
  solar.currentA = packet.currentMa / 1000.0f;
  solar.powerW = packet.powerCw / 100.0f;
  solar.capacityAh = packet.capacityCAh / 100.0f;
  solar.energyWh = packet.energyCWh / 100.0f;
  solar.temperatureC = packet.tempDc / 10.0f;
  solar.runtimeS = packet.runtimeS;
  solar.frameCount = packet.frameCount;
  solar.seq = packet.seq;
  solar.receivedAtMs = millis();
  util::copyCString(solar.sppTarget, sizeof(solar.sppTarget), packet.sppTarget);
  util::copyCString(solar.lastError, sizeof(solar.lastError), packet.lastError);
  if (packet.version >= 2) {
    solar.bridgeEspValid = true;
    solar.bridgeEsp.cpuCore0Percent = packet.cpu0Percent;
    solar.bridgeEsp.cpuCore1Percent = packet.cpu1Percent;
    solar.bridgeEsp.chipTemperatureC = packet.chipTempDc / 10.0f;
    solar.bridgeEsp.freeHeapBytes = static_cast<uint32_t>(packet.heapKb) * 1024u;
    solar.bridgeEsp.uptimeSeconds = packet.uptimeSec;
  }
  store.updateSolar(solar);
}

void copyEcoFlowDetail(telemetry::EcoFlowBridgeTelemetry& eco,
                       const ecoflow_bridge::EspNowPacketV1& packet) {
  eco.inSumW = packet.inSumW;
  eco.outSumW = packet.outSumW;
  eco.bmsPowerW = packet.bmsPowerW;
  eco.sohPercent = packet.sohPercent;
  eco.acInFreqHz = packet.acInFreqHz;
  eco.chgDsgState = packet.chgDsgState;
  eco.fanLevel = packet.fanLevel;
  eco.chargeLimitPercent = packet.chargeLimitPercent;
  eco.dischargeLimitPercent = packet.dischargeLimitPercent;
  eco.minCellTempC = packet.minCellTempC;
  eco.maxCellTempC = packet.maxCellTempC;
  eco.minMosTempC = packet.minMosTempC;
  eco.maxMosTempC = packet.maxMosTempC;
  eco.errCode = packet.errCode;
  const uint8_t extra = packet.extraFlags;
  eco.acInputOn = (extra & ecoflow_bridge::kExtraAcInputOn) != 0;
  eco.pvInputOn = (extra & ecoflow_bridge::kExtraPvInputOn) != 0;
  eco.haveCellTemp = (extra & ecoflow_bridge::kExtraCellTemp) != 0;
  eco.haveSoh = (extra & ecoflow_bridge::kExtraSoh) != 0;
  eco.haveFreq = (extra & ecoflow_bridge::kExtraFreq) != 0;
  eco.haveErr = (extra & ecoflow_bridge::kExtraErr) != 0;
  eco.haveChgState = (extra & ecoflow_bridge::kExtraChgState) != 0;
  eco.haveFan = (extra & ecoflow_bridge::kExtraFan) != 0;
  if (packet.version < 3) {
    return;
  }
  eco.acInVoltsDeci = packet.acInVoltsDeci;
  eco.acOutVoltsDeci = packet.acOutVoltsDeci;
  eco.acInCentiamps = packet.acInCentiamps;
  eco.acOutCentiamps = packet.acOutCentiamps;
  eco.bmsVoltsCenti = packet.bmsVoltsCenti;
  eco.bmsCentiamps = packet.bmsCentiamps;
  eco.minCellMv = packet.minCellMv;
  eco.maxCellMv = packet.maxCellMv;
  eco.fullCapMah = packet.fullCapMah;
  eco.remainCapMah = packet.remainCapMah;
  eco.pcsDcTempC = packet.pcsDcTempC;
  eco.pcsAcTempC = packet.pcsAcTempC;
  const uint8_t runtime = packet.runtimeFlags;
  eco.haveAcInElec = (runtime & ecoflow_bridge::kRuntimeAcIn) != 0;
  eco.haveAcOutElec = (runtime & ecoflow_bridge::kRuntimeAcOut) != 0;
  eco.haveBmsElec = (runtime & ecoflow_bridge::kRuntimeBms) != 0;
  eco.haveCellVolts = (runtime & ecoflow_bridge::kRuntimeCellVolts) != 0;
  eco.haveCapacity = (runtime & ecoflow_bridge::kRuntimeCapacity) != 0;
  eco.havePcsTemp = (runtime & ecoflow_bridge::kRuntimePcsTemp) != 0;
  eco.haveChargeLimit = (runtime & ecoflow_bridge::kRuntimeChgLimit) != 0;
  eco.haveDischargeLimit = (runtime & ecoflow_bridge::kRuntimeDsgLimit) != 0;
}

void applyEcoFlowPacket(telemetry::ITelemetryStore& store, bool encrypt,
                        const ecoflow_bridge::EspNowPacketV1& packet) {
  telemetry::EcoFlowBridgeTelemetry eco = {};
  eco.linkFresh = true;
  eco.bleConnected = (packet.flags & ecoflow_bridge::kFlagBleConnected) != 0;
  eco.meterValid = (packet.flags & ecoflow_bridge::kFlagTelemetryValid) != 0;
  eco.acOutputOn = (packet.flags & ecoflow_bridge::kFlagAcOutputOn) != 0;
  eco.dcOutputOn = (packet.flags & ecoflow_bridge::kFlagDcOutputOn) != 0;
  eco.usbOutputOn = (packet.flags & ecoflow_bridge::kFlagUsbOutputOn) != 0;
  eco.haveTemperature = (packet.flags & ecoflow_bridge::kFlagTempValid) != 0;
  eco.espNowEncrypted = encrypt || ((packet.flags & ecoflow_bridge::kFlagEncrypted) != 0);
  eco.socPercent = packet.socPercent;
  eco.acOutputW = packet.acOutputW;
  eco.acInputW = packet.acInputW;
  eco.dcOutputW = packet.dcOutputW;
  eco.solarInputW = packet.solarInputW;
  eco.usbOutputW = packet.usbOutputW;
  eco.temperatureC = eco.haveTemperature ? (packet.tempDc / 10.0f) : 0.0f;
  eco.remainMinutes = packet.remainMinutes;
  eco.seq = packet.seq;
  eco.telemetryCount = packet.telemetryCount;
  eco.receivedAtMs = millis();
  util::copyCString(eco.lastError, sizeof(eco.lastError), packet.lastError);
  if ((packet.flags & ecoflow_bridge::kFlagBridgeEspValid) != 0) {
    eco.bridgeEspValid = true;
    eco.bridgeEsp.cpuCore0Percent = packet.cpu0Percent;
    eco.bridgeEsp.cpuCore1Percent = packet.cpu1Percent;
    eco.bridgeEsp.chipTemperatureC = packet.chipTempDc / 10.0f;
    eco.bridgeEsp.freeHeapBytes = static_cast<uint32_t>(packet.heapKb) * 1024u;
    eco.bridgeEsp.uptimeSeconds = packet.uptimeSec;
  }
  if (packet.version >= 2) {
    copyEcoFlowDetail(eco, packet);
  }
  store.updateEcoFlow(eco);
}

void applyBmsTelemetryPacket(telemetry::ITelemetryStore& store,
                             const wattcycle_bridge::EspNowTelemetryPacketV1& packet) {
  bms::BatteryTelemetry battery = {};
  battery.valid = (packet.flags & wattcycle_bridge::kFlagBatteryValid) != 0;
  battery.stateOfChargePercent = packet.socPercent;
  battery.stateOfHealthPercent = packet.sohPercent;
  battery.moduleVoltage = packet.moduleVoltageCv / 100.0f;
  battery.currentAmps = packet.currentDa / 10.0f;
  battery.powerWatts = static_cast<float>(packet.powerW);
  battery.balanceCurrentAmps = packet.balanceCurrentDa / 10.0f;
  battery.remainingCapacityAh = packet.remainingCAh / 100.0f;
  battery.totalCapacityAh = packet.totalCAh / 100.0f;
  battery.designCapacityAh = packet.designCAh / 100.0f;
  battery.cycleNumber = packet.cycleNumber;
  battery.mosTemperatureC = packet.mosTempDc / 10.0f;
  battery.pcbTemperatureC = packet.pcbTempDc / 10.0f;
  battery.cellCount =
      packet.cellCount > wattcycle_bridge::kMaxCells ? wattcycle_bridge::kMaxCells : packet.cellCount;
  battery.temperatureCount = 2;
  for (uint8_t i = 0; i < battery.cellCount; ++i) {
    battery.cellVoltages[i] = packet.cellMv[i] / 1000.0f;
  }
  if (packet.version >= 2 && packet.cellSensorCount > 0) {
    const uint8_t sensorCount =
        packet.cellSensorCount > 4 ? static_cast<uint8_t>(4) : packet.cellSensorCount;
    battery.temperatureCount = static_cast<uint8_t>(2 + sensorCount);
    for (uint8_t i = 0; i < sensorCount; ++i) {
      battery.cellTemperaturesC[i] = packet.cellTempDc[i] / 10.0f;
    }
  }
  battery.updatedAtMs = millis();
  store.updateBattery(battery);
  store.setBmsLink(battery.valid,
                   (packet.flags & wattcycle_bridge::kFlagBleConnected) != 0);

  if ((packet.flags & wattcycle_bridge::kFlagBridgeEspValid) != 0) {
    telemetry::EspHealth bridgeEsp = {};
    bridgeEsp.cpuCore0Percent = packet.cpu0Percent;
    bridgeEsp.cpuCore1Percent = packet.cpu1Percent;
    bridgeEsp.chipTemperatureC = packet.chipTempDc / 10.0f;
    bridgeEsp.freeHeapBytes = static_cast<uint32_t>(packet.heapKb) * 1024u;
    bridgeEsp.uptimeSeconds = packet.uptimeSec;
    store.updateBmsBridgeHealth(bridgeEsp, true);
  }

  bms::WarningFlags warnings = {};
  warnings.valid = (packet.flags & wattcycle_bridge::kFlagWarningsValid) != 0;
  warnings.cellCount = battery.cellCount;
  warnings.hasActiveProtection = (packet.flags & wattcycle_bridge::kFlagHasProtection) != 0;
  warnings.hasFault = (packet.flags & wattcycle_bridge::kFlagHasFault) != 0;
  warnings.statusRegister1 = packet.statusRegister1;
  warnings.statusRegister2 = packet.statusRegister2;
  warnings.statusRegister5 = packet.statusRegister5;
  warnings.warningRegister1 = packet.warningRegister1;
  warnings.warningRegister2 = packet.warningRegister2;
  for (uint8_t i = 0; i < battery.cellCount; ++i) {
    warnings.cellBalancing[i] = (packet.balanceBits[i / 8u] & (1u << (i % 8u))) != 0;
  }
  store.updateWarnings(warnings);
}

void applyBmsProductPacket(telemetry::ITelemetryStore& store,
                           const wattcycle_bridge::EspNowProductPacketV1& packet) {
  bms::ProductInfo product = {};
  product.valid = true;
  util::copyCString(product.firmwareVersion, sizeof(product.firmwareVersion),
                    packet.firmwareVersion);
  util::copyCString(product.manufacturerName, sizeof(product.manufacturerName),
                    packet.manufacturerName);
  util::copyCString(product.serialNumber, sizeof(product.serialNumber), packet.serialNumber);
  store.updateProduct(product);
  store.setBleState(true, packet.bleAddress, packet.lastError);
}

}  // namespace wattcycle::espnow_rx
