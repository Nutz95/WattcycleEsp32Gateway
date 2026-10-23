#pragma once

#include "Shelly/IShellyPlugHistoryStore.h"
#include "Shelly/IShellyPlugHub.h"

#include <ctime>

namespace wattcycle::panel {

/// Appends authorized plug samples and streams day CSV from SD.
class P4ShellyHistory : public shelly::IShellyPlugHistoryStore {
 public:
  explicit P4ShellyHistory(shelly::IShellyPlugHub& hub);

  void append(std::time_t epochUtc, uint32_t nowMs);
  bool streamDay(const char* yyyymmdd, storage::HistoryStreamBegin begin,
                 storage::HistoryChunkSink sink, void* user) const override;

 private:
  shelly::IShellyPlugHub& hub_;
  uint32_t lastAppendMs_ = 0;
};

}  // namespace wattcycle::panel
