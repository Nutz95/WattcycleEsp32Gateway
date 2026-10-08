#pragma once

#include "Storage/IDailyHistoryStore.h"

namespace wattcycle::storage {

/// No-op history for hubs without SD (legacy TTGO).
class NullDailyHistoryStore : public IDailyHistoryStore {
 public:
  bool begin() override { return false; }
  bool isReady() const override { return false; }
  void append(const HistorySample& sample) override { (void)sample; }
  size_t listDays(char out[][11], size_t maxDays) const override {
    (void)out;
    (void)maxDays;
    return 0;
  }
  bool streamDay(const char* /*yyyymmdd*/, HistoryStreamBegin /*begin*/,
                 HistoryChunkSink /*sink*/, void* /*user*/) const override {
    return false;
  }
};

}  // namespace wattcycle::storage
