#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>

namespace wattcycle::storage {

struct HistorySample {
  std::time_t epochUtc = 0;
  uint8_t socPercent = 0;
  float packVoltageV = 0;
  float packCurrentA = 0;
  float packPowerW = 0;
  float solarVoltageV = 0;
  float solarCurrentA = 0;
  float solarPowerW = 0;
  bool packValid = false;
  bool solarValid = false;
};

/// Called once with total byte size before chunks (may be 0 if unknown).
using HistoryStreamBegin = bool (*)(size_t totalBytes, void* user);
/// Chunk sink for day CSV streaming. Return false to abort.
using HistoryChunkSink = bool (*)(const uint8_t* data, size_t length, void* user);

/// Day files on SD: /history/YYYY-MM-DD.csv (UTC date).
class IDailyHistoryStore {
 public:
  virtual ~IDailyHistoryStore() = default;
  virtual bool begin() = 0;
  virtual bool isReady() const = 0;
  virtual void append(const HistorySample& sample) = 0;
  /// Fills out[i] with "YYYY-MM-DD"; returns count written.
  virtual size_t listDays(char out[][11], size_t maxDays) const = 0;
  /// Stream day CSV via begin+sink (no Arduino File in the contract). False = missing/error.
  virtual bool streamDay(const char* yyyymmdd, HistoryStreamBegin begin, HistoryChunkSink sink,
                         void* user) const = 0;
};

}  // namespace wattcycle::storage
