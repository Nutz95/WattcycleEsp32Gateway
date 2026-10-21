#pragma once

#include <cstdint>

namespace ecoflow::models {

/// Latest read-only snapshot from DisplayPropertyUpload (partial merges OK).
struct PowerStationTelemetry {
  bool valid = false;
  uint8_t socPercent = 0;
  int16_t acOutputW = 0;
  int16_t acInputW = 0;
  int16_t dcOutputW = 0;
  int16_t solarInputW = 0;
  int16_t usbOutputW = 0;
  int16_t inSumW = 0;
  int16_t outSumW = 0;
  int16_t bmsPowerW = 0;
  int16_t tempC = 0;
  int16_t minCellTempC = 0;
  int16_t maxCellTempC = 0;
  int16_t minMosTempC = 0;
  int16_t maxMosTempC = 0;
  bool haveTemperature = false;
  bool haveCellTemp = false;
  bool haveSoh = false;
  bool haveFreq = false;
  bool haveChgState = false;
  bool haveFan = false;
  bool haveErr = false;
  bool haveChargeLimit = false;
  bool haveDischargeLimit = false;
  bool acInputOn = false;
  bool pvInputOn = false;
  bool haveAcInElec = false;
  bool haveAcOutElec = false;
  bool haveBmsElec = false;
  bool haveCellVolts = false;
  bool haveCapacity = false;
  bool havePcsTemp = false;
  int16_t acInVoltsDeci = 0;
  int16_t acOutVoltsDeci = 0;
  int16_t acInCentiamps = 0;
  int16_t acOutCentiamps = 0;
  int16_t bmsVoltsCenti = 0;
  int16_t bmsCentiamps = 0;
  uint16_t minCellMv = 0;
  uint16_t maxCellMv = 0;
  uint16_t fullCapMah = 0;
  uint16_t remainCapMah = 0;
  int16_t pcsDcTempC = 0;
  int16_t pcsAcTempC = 0;
  uint16_t remainMinutes = 0;
  uint8_t sohPercent = 0;
  uint8_t acInFreqHz = 0;
  uint8_t chgDsgState = 0;
  uint8_t fanLevel = 0;
  uint8_t chargeLimitPercent = 0;
  uint8_t dischargeLimitPercent = 0;
  bool acOutputOn = false;
  bool dcOutputOn = false;
  bool usbOutputOn = false;
  uint32_t errCode = 0;
  uint32_t updatedAtMs = 0;
};

}  // namespace ecoflow::models
