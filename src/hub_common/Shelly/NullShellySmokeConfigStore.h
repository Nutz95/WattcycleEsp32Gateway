#pragma once

#include "Shelly/IShellySmokeConfigStore.h"

namespace wattcycle::shelly {

/// No-op store for tests and hosts without an SD card.
class NullShellySmokeConfigStore : public IShellySmokeConfigStore {
 public:
  size_t loadAuthorized(ShellyDeviceIdentity* /*out*/, size_t /*capacity*/) override {
    return 0;
  }
  bool saveAuthorized(const ShellyDeviceIdentity* /*entries*/, size_t /*count*/) override {
    return true;
  }
};

}  // namespace wattcycle::shelly
