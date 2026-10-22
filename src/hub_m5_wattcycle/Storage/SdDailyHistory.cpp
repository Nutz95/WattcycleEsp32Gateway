#include "Storage/SdDailyHistory.h"

#include "Bus/SharedSpiLock.h"
#include "Storage/HistoryCatalog.h"
#include "Storage/HistoryDayFile.h"

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
  bus::SpiGuard guard;
  HistoryDayFile::appendSample(SD, sample);
}

size_t SdDailyHistory::listDays(char out[][11], size_t maxDays) const {
  if (!ready_ || out == nullptr || maxDays == 0) {
    return 0;
  }
  bus::SpiGuard guard;
  return HistoryDayFile::listDays(SD, out, maxDays);
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
  return HistoryDayFile::streamDay(SD, yyyymmdd, begin, sink, user);
}

}  // namespace wattcycle::storage
