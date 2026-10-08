#include "Storage/SdDailyHistory.h"

#include "Bus/SharedSpiLock.h"

#include <cstdio>
#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#endif

namespace wattcycle::storage {
namespace {
#ifndef UNIT_TEST
constexpr int kSdCs = 4;
constexpr int kSdSck = 18;
constexpr int kSdMiso = 19;
constexpr int kSdMosi = 23;
constexpr uint32_t kMinAppendIntervalMs = 10000;
constexpr const char* kHistoryDir = "/history";
constexpr size_t kStreamChunkBytes = 512;

bool isYmd(const char* name) {
  if (name == nullptr || std::strlen(name) != 10) {
    return false;
  }
  for (int i = 0; i < 10; ++i) {
    if (i == 4 || i == 7) {
      if (name[i] != '-') {
        return false;
      }
    } else if (name[i] < '0' || name[i] > '9') {
      return false;
    }
  }
  return true;
}
#endif
}  // namespace

bool SdDailyHistory::begin() {
#ifndef UNIT_TEST
  bus::SharedSpiLock::begin();
  bus::SpiGuard guard;
  SPI.begin(kSdSck, kSdMiso, kSdMosi, kSdCs);
  if (!SD.begin(kSdCs, SPI, 25000000)) {
    Serial.println(F("SD card not detected — daily history off"));
    ready_ = false;
    return false;
  }
  if (!SD.exists(kHistoryDir) && !SD.mkdir(kHistoryDir)) {
    Serial.println(F("SD mkdir /history failed"));
    ready_ = false;
    return false;
  }
  ready_ = true;
  Serial.println(F("SD daily history ready (/history/YYYY-MM-DD.csv)"));
  return true;
#else
  ready_ = true;
  return true;
#endif
}

bool SdDailyHistory::isReady() const {
  return ready_;
}

void SdDailyHistory::append(const HistorySample& sample) {
#ifndef UNIT_TEST
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
  char date[11] = {};
  std::snprintf(date, sizeof(date), "%04d-%02d-%02d", info.tm_year + 1900, info.tm_mon + 1,
                info.tm_mday);
  char path[32] = {};
  std::snprintf(path, sizeof(path), "%s/%s.csv", kHistoryDir, date);

  bus::SpiGuard guard;
  const bool exists = SD.exists(path);
  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    return;
  }
  if (!exists) {
    file.println(
        F("epoch,soc,pack_v,pack_a,pack_w,solar_v,solar_a,solar_w,pack_ok,solar_ok"));
  }
  file.printf("%ld,%u,%.3f,%.3f,%.1f,%.3f,%.3f,%.1f,%u,%u\n",
              static_cast<long>(sample.epochUtc), sample.socPercent, sample.packVoltageV,
              sample.packCurrentA, sample.packPowerW, sample.solarVoltageV, sample.solarCurrentA,
              sample.solarPowerW, sample.packValid ? 1u : 0u, sample.solarValid ? 1u : 0u);
  file.close();
#else
  (void)sample;
#endif
}

size_t SdDailyHistory::listDays(char out[][11], size_t maxDays) const {
#ifndef UNIT_TEST
  if (!ready_ || out == nullptr || maxDays == 0) {
    return 0;
  }
  bus::SpiGuard guard;
  File dir = SD.open(kHistoryDir);
  if (!dir || !dir.isDirectory()) {
    return 0;
  }
  size_t count = 0;
  for (File entry = dir.openNextFile(); entry && count < maxDays; entry = dir.openNextFile()) {
    const char* name = entry.name();
    const char* base = name;
    if (const char* slash = std::strrchr(name, '/')) {
      base = slash + 1;
    }
    char ymd[11] = {};
    if (std::strlen(base) >= 10) {
      std::memcpy(ymd, base, 10);
      ymd[10] = '\0';
    }
    if (isYmd(ymd)) {
      std::memcpy(out[count], ymd, 11);
      ++count;
    }
    entry.close();
  }
  dir.close();
  return count;
#else
  (void)out;
  (void)maxDays;
  return 0;
#endif
}

bool SdDailyHistory::streamDay(const char* yyyymmdd, HistoryStreamBegin begin,
                               HistoryChunkSink sink, void* user) const {
#ifndef UNIT_TEST
  if (!ready_ || yyyymmdd == nullptr || begin == nullptr || sink == nullptr || !isYmd(yyyymmdd)) {
    return false;
  }
  char path[32] = {};
  std::snprintf(path, sizeof(path), "%s/%s.csv", kHistoryDir, yyyymmdd);

  bus::SpiGuard guard;
  File file = SD.open(path, FILE_READ);
  if (!file) {
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
#else
  (void)yyyymmdd;
  (void)begin;
  (void)sink;
  (void)user;
  return false;
#endif
}

}  // namespace wattcycle::storage
