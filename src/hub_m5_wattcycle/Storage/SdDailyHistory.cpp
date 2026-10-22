#include "Storage/SdDailyHistory.h"

#include "Bus/SharedSpiLock.h"
#include "Storage/HistoryCatalog.h"

#include <cstdio>

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

namespace wattcycle::storage {
namespace {
constexpr int kSdCs = 4;
constexpr int kSdSck = 18;
constexpr int kSdMiso = 19;
constexpr int kSdMosi = 23;
constexpr uint32_t kMinAppendIntervalMs = 10000;
constexpr size_t kStreamChunkBytes = 512;
}  // namespace

bool SdDailyHistory::begin() {
  bus::SharedSpiLock::begin();
  bus::SpiGuard guard;
  SPI.begin(kSdSck, kSdMiso, kSdMosi, kSdCs);
  if (!SD.begin(kSdCs, SPI, 25000000)) {
    Serial.println(F("SD card not detected — daily history off"));
    ready_ = false;
    return false;
  }
  if (!SD.exists("/history") && !SD.mkdir("/history")) {
    Serial.println(F("SD mkdir /history failed"));
    ready_ = false;
    return false;
  }
  ready_ = true;
  Serial.println(F("SD daily history ready (/history/YYYY/MM/YYYY-MM-DD.csv)"));
  return true;
}

bool SdDailyHistory::isReady() const {
  return ready_;
}

void SdDailyHistory::append(const HistorySample& sample) {
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
  char path[40] = {};
  HistoryCatalog::formatDayPath(path, sizeof(path), date);

  bus::SpiGuard guard;
  if (!HistoryCatalog::ensureMonth(SD, year, month)) {
    return;
  }
  const bool exists = SD.exists(path);
  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    return;
  }
  if (!exists) {
    file.println(F("epoch,soc,pack_v,pack_a,pack_w,solar_v,solar_a,solar_w,pack_ok,solar_ok,"
                   "pack_ah,mos_c,pcb_c,solar_wh,solar_ah"));
  }
  file.printf("%ld,%u,%.3f,%.3f,%.1f,%.3f,%.3f,%.1f,%u,%u,%.2f,%.1f,%.1f,%.3f,%.3f\n",
              static_cast<long>(sample.epochUtc), sample.socPercent, sample.packVoltageV,
              sample.packCurrentA, sample.packPowerW, sample.solarVoltageV, sample.solarCurrentA,
              sample.solarPowerW, sample.packValid ? 1u : 0u, sample.solarValid ? 1u : 0u,
              sample.packRemainingAh, sample.packMosTempC, sample.packPcbTempC,
              sample.solarEnergyWh, sample.solarCapacityAh);
  file.close();
}

size_t SdDailyHistory::listDays(char out[][11], size_t maxDays) const {
  if (!ready_ || out == nullptr || maxDays == 0) {
    return 0;
  }
  bus::SpiGuard guard;
  char periods[48][8] = {};
  const size_t periodCount = HistoryCatalog::listPeriods(SD, periods, 48);
  size_t count = 0;
  for (size_t i = 0; i < periodCount && count < maxDays; ++i) {
    int year = 0;
    int month = 0;
    if (std::sscanf(periods[i], "%d-%d", &year, &month) != 2) {
      continue;
    }
    count += HistoryCatalog::listMonthDays(SD, year, month, out + count, maxDays - count);
  }
  return count;
}

size_t SdDailyHistory::listPeriods(char out[][8], size_t maxPeriods) const {
  if (!ready_) {
    return 0;
  }
  bus::SpiGuard guard;
  return HistoryCatalog::listPeriods(SD, out, maxPeriods);
}

size_t SdDailyHistory::listMonthDays(int year, int month, char out[][11], size_t maxDays) const {
  if (!ready_) {
    return 0;
  }
  bus::SpiGuard guard;
  return HistoryCatalog::listMonthDays(SD, year, month, out, maxDays);
}

bool SdDailyHistory::streamDay(const char* yyyymmdd, HistoryStreamBegin begin,
                               HistoryChunkSink sink, void* user) const {
  if (!ready_ || yyyymmdd == nullptr || begin == nullptr || sink == nullptr) {
    return false;
  }
  bus::SpiGuard guard;
  File file;
  if (!HistoryCatalog::openDay(SD, yyyymmdd, file)) {
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

}  // namespace wattcycle::storage
