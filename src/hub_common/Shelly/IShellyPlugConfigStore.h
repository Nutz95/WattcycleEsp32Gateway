#pragma once

#include "Shelly/ShellyPlugIdentity.h"

#include <cstddef>
#include <cstdint>

namespace wattcycle::shelly {

/// Persist whitelist and day baselines (SD on the P4).
class IShellyPlugConfigStore {
 public:
  virtual ~IShellyPlugConfigStore() = default;
  /// Load whitelist entries. Returns count written into `out`.
  virtual size_t loadAuthorized(ShellyPlugIdentity* out, size_t capacity) = 0;
  virtual bool saveAuthorized(const ShellyPlugIdentity* entries, size_t count) = 0;
  /// Day key YYYYMMDD UTC and baseline Wh per id. Missing id leaves baseline unset.
  virtual bool loadDayBaseline(uint32_t& dayYyyymmdd, float* whBySlot, char ids[][kShellyIdCapacity],
                               size_t capacity, size_t& count) = 0;
  virtual bool saveDayBaseline(uint32_t dayYyyymmdd, const float* whBySlot,
                               const char ids[][kShellyIdCapacity], size_t count) = 0;
};

}  // namespace wattcycle::shelly
