#pragma once

#include "Shelly/IShellySmokeConfigStore.h"

namespace wattcycle::panel {

/// Whitelist in `/smoke.cfg` on the SD card.
class P4ShellySmokeConfigStore : public shelly::IShellySmokeConfigStore {
 public:
  size_t loadAuthorized(shelly::ShellyDeviceIdentity* out, size_t capacity) override;
  bool saveAuthorized(const shelly::ShellyDeviceIdentity* entries, size_t count) override;
};

}  // namespace wattcycle::panel
