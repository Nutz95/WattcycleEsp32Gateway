#pragma once

#include "Storage/IDailyHistoryStore.h"

#include <cstdint>

namespace wattcycle::storage {

class SdDailyHistory : public IDailyHistoryStore {
 public:
  bool begin() override;
  bool isReady() const override;
  void append(const HistorySample& sample) override;
  size_t listDays(char out[][11], size_t maxDays) const override;
  size_t listPeriods(char out[][8], size_t maxPeriods) const override;
  size_t listMonthDays(int year, int month, char out[][11], size_t maxDays) const override;
  bool streamDay(const char* yyyymmdd, HistoryStreamBegin begin, HistoryChunkSink sink,
                 void* user) const override;

 private:
  bool ready_ = false;
  uint32_t lastAppendMs_ = 0;
};

}  // namespace wattcycle::storage
