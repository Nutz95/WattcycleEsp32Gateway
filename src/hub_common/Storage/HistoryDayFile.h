#pragma once

#include "Storage/IDailyHistoryStore.h"

#include <FS.h>

#include <cstddef>

namespace wattcycle::storage {

/// One CSV day: append a row, list every day, or stream the bytes.
class HistoryDayFile {
 public:
  /// Every stored day, newest period first.
  static size_t listDays(fs::FS& fs, char out[][11], size_t maxDays);
  /// Append one CSV row. Creates the month folder and the header when needed.
  static void appendSample(fs::FS& fs, const HistorySample& sample);
  /// Stream one day. False when the file is missing or a sink returns false.
  static bool streamDay(fs::FS& fs, const char* yyyymmdd, HistoryStreamBegin begin,
                        HistoryChunkSink sink, void* user);
};

}  // namespace wattcycle::storage
