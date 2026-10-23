#include "P4ShellyHistory.h"

#include "Shelly/ShellyPlugLimits.h"
#include "Shelly/ShellyPlugReading.h"

#include <SD_MMC.h>

#include <cstdio>
#include <ctime>
#include <cstring>

namespace wattcycle::panel {
namespace {

bool ensureDir(const char* path) {
  if (SD_MMC.exists(path)) {
    return true;
  }
  return SD_MMC.mkdir(path);
}

bool dayPathFromEpoch(std::time_t epochUtc, char* out, size_t capacity) {
  const std::tm* utc = std::gmtime(&epochUtc);
  if (utc == nullptr || out == nullptr || capacity < 40) {
    return false;
  }
  std::snprintf(out, capacity, "/history/plugs/%04d-%02d-%02d.csv", utc->tm_year + 1900,
                utc->tm_mon + 1, utc->tm_mday);
  return true;
}

bool dayPathFromStamp(const char* yyyymmdd, char* out, size_t capacity) {
  if (yyyymmdd == nullptr || std::strlen(yyyymmdd) != 10 || out == nullptr || capacity < 40) {
    return false;
  }
  std::snprintf(out, capacity, "/history/plugs/%s.csv", yyyymmdd);
  return true;
}

}  // namespace

P4ShellyHistory::P4ShellyHistory(shelly::IShellyPlugHub& hub) : hub_(hub) {}

void P4ShellyHistory::append(std::time_t epochUtc, uint32_t nowMs) {
  if (epochUtc <= 0 || SD_MMC.cardType() == CARD_NONE) {
    return;
  }
  if (lastAppendMs_ != 0 && static_cast<uint32_t>(nowMs - lastAppendMs_) < 30000u) {
    return;
  }
  shelly::ShellyPlugReading readings[shelly::kMaxShellyPlugs] = {};
  const size_t count = hub_.snapshotAuthorized(readings, shelly::kMaxShellyPlugs, nowMs);
  if (count == 0) {
    return;
  }
  if (!ensureDir("/history") || !ensureDir("/history/plugs")) {
    return;
  }
  char path[48] = {};
  if (!dayPathFromEpoch(epochUtc, path, sizeof(path))) {
    return;
  }
  const bool createHeader = !SD_MMC.exists(path);
  File file = SD_MMC.open(path, createHeader ? "w" : "a");
  if (!file) {
    return;
  }
  if (createHeader) {
    file.println("epoch,id,name,watts,wh_total,wh_today,output");
  }
  bool wrote = false;
  for (size_t i = 0; i < count; ++i) {
    if (!readings[i].online) {
      continue;
    }
    char line[160] = {};
    std::snprintf(line, sizeof(line), "%ld,%s,%s,%.1f,%.3f,%.3f,%d",
                  static_cast<long>(epochUtc), readings[i].id, readings[i].name, readings[i].watts,
                  readings[i].whTotal, readings[i].whToday, readings[i].output ? 1 : 0);
    file.println(line);
    wrote = true;
  }
  file.close();
  if (wrote) {
    lastAppendMs_ = nowMs;
  }
}

bool P4ShellyHistory::streamDay(const char* yyyymmdd, storage::HistoryStreamBegin begin,
                                storage::HistoryChunkSink sink, void* user) const {
  if (begin == nullptr || sink == nullptr || SD_MMC.cardType() == CARD_NONE) {
    return false;
  }
  char path[48] = {};
  if (!dayPathFromStamp(yyyymmdd, path, sizeof(path))) {
    return false;
  }
  File file = SD_MMC.open(path, "r");
  if (!file) {
    return false;
  }
  if (!begin(file.size(), user)) {
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

}  // namespace wattcycle::panel
