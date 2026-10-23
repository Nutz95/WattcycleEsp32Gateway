#pragma once

#include "Shelly/IShellyPlugHistoryStore.h"

namespace wattcycle::shelly {

/// No-op day reader for hubs without Shelly history.
class NullShellyPlugHistoryStore : public IShellyPlugHistoryStore {
 public:
  bool streamDay(const char*, storage::HistoryStreamBegin, storage::HistoryChunkSink,
                 void*) const override {
    return false;
  }
};

}  // namespace wattcycle::shelly
