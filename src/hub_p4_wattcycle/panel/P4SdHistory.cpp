#include "P4SdHistory.h"

#include "Storage/HistoryCatalog.h"
#include "Storage/HistoryDayFile.h"

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

namespace wattcycle::panel {
namespace {

constexpr int kClk = 43;
constexpr int kCmd = 44;
constexpr int kD0 = 39;
constexpr int kD1 = 40;
constexpr int kD2 = 41;
constexpr int kD3 = 42;
constexpr uint32_t kMinAppendIntervalMs = 10000;

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
  storage::HistoryDayFile::appendSample(SD_MMC, sample);
}

size_t P4SdHistory::listDays(char out[][11], size_t maxDays) const {
  if (!ready_ || out == nullptr || maxDays == 0) {
    return 0;
  }
  return storage::HistoryDayFile::listDays(SD_MMC, out, maxDays);
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
  return storage::HistoryDayFile::streamDay(SD_MMC, yyyymmdd, begin, sink, user);
}

bool P4SdHistory::storage(uint32_t& usedMb, uint32_t& totalMb) const {
  usedMb = 0;
  totalMb = 0;
  if (!ready_) {
    return false;
  }
  const uint64_t total = SD_MMC.totalBytes();
  if (total == 0) {
    return false;
  }
  totalMb = static_cast<uint32_t>(total / (1024ull * 1024ull));
  usedMb = static_cast<uint32_t>(SD_MMC.usedBytes() / (1024ull * 1024ull));
  return true;
}

}  // namespace wattcycle::panel
