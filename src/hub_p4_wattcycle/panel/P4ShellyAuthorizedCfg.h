#pragma once

#include "Shelly/ShellyDeviceIdentity.h"

#include <cstddef>

namespace wattcycle::panel {

/// Load `v1` / `id=name` whitelist lines from an SD path.
size_t loadShellyAuthorizedCfg(const char* path, shelly::ShellyDeviceIdentity* out, size_t capacity);
/// Replace a whitelist file with `v1` and `id=name` lines.
bool saveShellyAuthorizedCfg(const char* path, const shelly::ShellyDeviceIdentity* entries,
                             size_t count);

}  // namespace wattcycle::panel
