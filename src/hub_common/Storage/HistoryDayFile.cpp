#include "Storage/HistoryDayFile.h"

#include "Storage/HistoryCatalog.h"

#include <Arduino.h>

#include <cstdio>

namespace wattcycle::storage {

size_t HistoryDayFile::listDays(fs::FS& fs, char out[][11], size_t maxDays) {
  if (out == nullptr || maxDays == 0) {
    return 0;
  }
  char periods[48][8] = {};
  const size_t periodCount = HistoryCatalog::listPeriods(fs, periods, 48);
  size_t count = 0;
  for (size_t i = 0; i < periodCount && count < maxDays; ++i) {
    int year = 0;
    int month = 0;
    if (std::sscanf(periods[i], "%d-%d", &year, &month) != 2) {
      continue;
    }
    count += HistoryCatalog::listMonthDays(fs, year, month, out + count, maxDays - count);
  }
  return count;
}

void HistoryDayFile::appendSample(fs::FS& fs, const HistorySample& sample) {
  if (sample.epochUtc == 0) {
    return;
  }
  struct tm info = {};
  if (gmtime_r(&sample.epochUtc, &info) == nullptr) {
    return;
  }
  const int year = info.tm_year + 1900;
  const int month = info.tm_mon + 1;
  char date[11] = {};
  std::snprintf(date, sizeof(date), "%04d-%02d-%02d", year, month, info.tm_mday);
  if (!HistoryCatalog::ensureMonth(fs, year, month)) {
    return;
  }
  char path[40] = {};
  HistoryCatalog::formatDayPath(path, sizeof(path), date);
  const bool exists = fs.exists(path);
  fs::File file = fs.open(path, FILE_APPEND);
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
              sample.packRemainingAh, sample.packMosTempC, sample.packPcbTempC, sample.solarEnergyWh,
              sample.solarCapacityAh);
  file.close();
}

bool HistoryDayFile::streamDay(fs::FS& fs, const char* yyyymmdd, HistoryStreamBegin begin,
                               HistoryChunkSink sink, void* user) {
  if (yyyymmdd == nullptr || begin == nullptr || sink == nullptr) {
    return false;
  }
  fs::File file;
  if (!HistoryCatalog::openDay(fs, yyyymmdd, file)) {
    return false;
  }
  if (!begin(static_cast<size_t>(file.size()), user)) {
    file.close();
    return false;
  }
  uint8_t chunk[512];
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
