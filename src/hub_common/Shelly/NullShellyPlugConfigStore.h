#pragma once

#include "Shelly/IShellyPlugConfigStore.h"

namespace wattcycle::shelly {

/// No-op store for tests and hosts without an SD card.
class NullShellyPlugConfigStore : public IShellyPlugConfigStore {
 public:
  size_t loadAuthorized(ShellyPlugIdentity* /*out*/, size_t /*capacity*/) override {
    return 0;
  }
  bool saveAuthorized(const ShellyPlugIdentity* /*entries*/, size_t /*count*/) override {
    return true;
  }
  bool loadDayBaseline(uint32_t& dayYyyymmdd, float* /*whBySlot*/,
                       char /*ids*/[][kShellyIdCapacity], size_t /*capacity*/,
                       size_t& count) override {
    dayYyyymmdd = 0;
    count = 0;
    return false;
  }
  bool saveDayBaseline(uint32_t /*dayYyyymmdd*/, const float* /*whBySlot*/,
                       const char /*ids*/[][kShellyIdCapacity], size_t /*count*/) override {
    return true;
  }
};

}  // namespace wattcycle::shelly
