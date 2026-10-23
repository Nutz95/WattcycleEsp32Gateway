#pragma once

#include "Storage/IDailyHistoryStore.h"

namespace wattcycle::shelly {

/// Day CSV under /history/plugs/YYYY-MM-DD.csv (no Arduino File in the contract).
class IShellyPlugHistoryStore {
 public:
  virtual ~IShellyPlugHistoryStore() = default;

  /// Stream day CSV via begin+sink. False = missing / SD down / bad date.
  virtual bool streamDay(const char* yyyymmdd, storage::HistoryStreamBegin begin,
                         storage::HistoryChunkSink sink, void* user) const = 0;
};

}  // namespace wattcycle::shelly
