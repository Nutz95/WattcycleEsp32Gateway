#pragma once

#include "Shelly/IShellyPlugConfigStore.h"

namespace wattcycle::panel {

/// Whitelist in `/plugs.cfg` and day baselines in `/plugs.day` on the SD card.
class P4ShellyConfigStore : public shelly::IShellyPlugConfigStore {
 public:
  size_t loadAuthorized(shelly::ShellyPlugIdentity* out, size_t capacity) override;
  bool saveAuthorized(const shelly::ShellyPlugIdentity* entries, size_t count) override;
  bool loadDayBaseline(uint32_t& dayYyyymmdd, float* whBySlot,
                       char ids[][shelly::kShellyIdCapacity], size_t capacity,
                       size_t& count) override;
  bool saveDayBaseline(uint32_t dayYyyymmdd, const float* whBySlot,
                       const char ids[][shelly::kShellyIdCapacity], size_t count) override;
};

}  // namespace wattcycle::panel
