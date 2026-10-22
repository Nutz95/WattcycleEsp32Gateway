#include "P4SdHistory.h"

#include "Storage/HistoryCatalog.h"

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

#include <cstdio>

namespace wattcycle::panel {
namespace {

constexpr int kClk = 43;
constexpr int kCmd = 44;
constexpr int kD0 = 39;
constexpr int kD1 = 40;
constexpr int kD2 = 41;
constexpr int kD3 = 42;
constexpr uint32_t kMinAppendIntervalMs = 10000;
constexpr size_t kStreamChunkBytes = 512;

bool mountFat() {
  if (!SD_MMC.setPins(kClk, kCmd, kD0, kD1, kD2, kD3)) {
    return false;
  }
  if (SD_MMC.begin("/sdcard", false)) {
    return true;
  }
  SD_MMC.end();
  if (!SD_MMC.setPins(kClk, kCmd, kD0)) {
    return false;
  }
  return SD_MMC.begin("/sdcard", true);
}

}  // namespace

bool P4SdHistory::begin() {
  if (!mountFat()) {
    Serial.println("SD card not detected");
    ready_ = false;
    return false;
  }
  if (!SD_MMC.exists("/history") && !SD_MMC.mkdir("/history")) {
    Serial.println("SD mkdir /history failed");
    ready_ = false;
    return false;
  }
  ready_ = true;
  Serial.println("SD daily history ready");
  return true;
}

bool P4SdHistory::isReady() const {
  return ready_;
}

void P4SdHistory::append(const storage::HistorySample& sample) {
  if (!ready_ || sample.epochUtc == 0) {
    return;
  }
  const uint32_t nowMs = millis();
  if (lastAppendMs_ != 0 && (nowMs - lastAppendMs_) < kMinAppendIntervalMs) {
    return;
  }
  lastAppendMs_ = nowMs;
  struct tm info = {};
  if (gmtime_r(&sample.epochUtc, &info) == nullptr) {
    return;
  }
  const int year = info.tm_year + 1900;
  const int month = info.tm_mon + 1;
  char date[11] = {};
  std::snprintf(date, sizeof(date), "%04d-%02d-%02d", year, month, info.tm_mday);
  if (!storage::HistoryCatalog::ensureMonth(SD_MMC, year, month)) {
    return;
  }
  char path[40] = {};
  storage::HistoryCatalog::formatDayPath(path, sizeof(path), date);
  const bool exists = SD_MMC.exists(path);
  File file = SD_MMC.open(path, FILE_APPEND);
  if (!file) {
    return;
  }
  if (!exists) {
    file.println("epoch,soc,pack_v,pack_a,pack_w,solar_v,solar_a,solar_w,pack_ok,solar_ok,"
                 "pack_ah,mos_c,pcb_c,solar_wh,solar_ah");
  }
  file.printf("%ld,%u,%.3f,%.3f,%.1f,%.3f,%.3f,%.1f,%u,%u,%.2f,%.1f,%.1f,%.3f,%.3f\n",
              static_cast<long>(sample.epochUtc), sample.socPercent, sample.packVoltageV,
              sample.packCurrentA, sample.packPowerW, sample.solarVoltageV, sample.solarCurrentA,
              sample.solarPowerW, sample.packValid ? 1u : 0u, sample.solarValid ? 1u : 0u,
              sample.packRemainingAh, sample.packMosTempC, sample.packPcbTempC,
              sample.solarEnergyWh, sample.solarCapacityAh);
  file.close();
}

size_t P4SdHistory::listDays(char out[][11], size_t maxDays) const {
  if (!ready_ || out == nullptr || maxDays == 0) {
    return 0;
  }
  char periods[48][8] = {};
  const size_t periodCount = storage::HistoryCatalog::listPeriods(SD_MMC, periods, 48);
  size_t count = 0;
  for (size_t i = 0; i < periodCount && count < maxDays; ++i) {
    int year = 0;
    int month = 0;
    if (std::sscanf(periods[i], "%d-%d", &year, &month) != 2) {
      continue;
    }
    count += storage::HistoryCatalog::listMonthDays(SD_MMC, year, month, out + count,
                                                   maxDays - count);
  }
  return count;
}

size_t P4SdHistory::listPeriods(char out[][8], size_t maxPeriods) const {
  if (!ready_) {
    return 0;
  }
  return storage::HistoryCatalog::listPeriods(SD_MMC, out, maxPeriods);
}

size_t P4SdHistory::listMonthDays(int year, int month, char out[][11], size_t maxDays) const {
  if (!ready_) {
    return 0;
  }
  return storage::HistoryCatalog::listMonthDays(SD_MMC, year, month, out, maxDays);
}

bool P4SdHistory::streamDay(const char* yyyymmdd, storage::HistoryStreamBegin begin,
                            storage::HistoryChunkSink sink, void* user) const {
  if (!ready_ || yyyymmdd == nullptr || begin == nullptr || sink == nullptr) {
    return false;
  }
  File file;
  if (!storage::HistoryCatalog::openDay(SD_MMC, yyyymmdd, file)) {
    return false;
  }
  if (!begin(static_cast<size_t>(file.size()), user)) {
    file.close();
    return false;
  }
  uint8_t chunk[kStreamChunkBytes];
  while (file.available()) {
    const int read = file.read(chunk, sizeof(chunk));
    if (read <= 0) {
      break;
    }
    if (!sink(chunk, static_cast<size_t>(read), user)) {
      file.close();
      return false;
    }
  }
  file.close();
  return true;
}

}  // namespace wattcycle::panel
