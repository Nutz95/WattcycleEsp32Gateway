#pragma once

#include "Shelly/ShellyDeviceIdentity.h"

#include <cstddef>

namespace wattcycle::shelly {

/// Persist smoke detector whitelist (SD on the P4).
class IShellySmokeConfigStore {
 public:
  virtual ~IShellySmokeConfigStore() = default;
  /// Load whitelist entries. Returns count written into `out`.
  virtual size_t loadAuthorized(ShellyDeviceIdentity* out, size_t capacity) = 0;
  /// Replace the persisted whitelist.
  virtual bool saveAuthorized(const ShellyDeviceIdentity* entries, size_t count) = 0;
};

}  // namespace wattcycle::shelly
