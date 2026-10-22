#pragma once
#ifndef P4_SD_HISTORY_H
#define P4_SD_HISTORY_H

#include "Storage/IDailyHistoryStore.h"

namespace wattcycle::panel {

/// Day CSV files on the panel TF slot. The card stays FAT32; nothing is reformatted.
class P4SdHistory : public storage::IDailyHistoryStore {
 public:
  /// Mount the SDMMC slot. False when no card answers.
  bool begin() override;
  bool isReady() const override;
  void append(const storage::HistorySample& sample) override;
  size_t listDays(char out[][11], size_t maxDays) const override;
  /// "YYYY-MM" folders, newest first, including legacy flat files.
  size_t listPeriods(char out[][8], size_t maxPeriods) const override;
  /// Day names for one UTC month, newest first.
  size_t listMonthDays(int year, int month, char out[][11], size_t maxDays) const override;
  bool streamDay(const char* yyyymmdd, storage::HistoryStreamBegin begin,
                 storage::HistoryChunkSink sink, void* user) const override;

 private:
  bool ready_ = false;
  uint32_t lastAppendMs_ = 0;
};

}  // namespace wattcycle::panel

#endif
