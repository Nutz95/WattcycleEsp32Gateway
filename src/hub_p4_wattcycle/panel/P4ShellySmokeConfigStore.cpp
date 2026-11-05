#include "P4ShellySmokeConfigStore.h"

#include "P4ShellyAuthorizedCfg.h"

namespace wattcycle::panel {

size_t P4ShellySmokeConfigStore::loadAuthorized(shelly::ShellyDeviceIdentity* out, size_t capacity) {
  return loadShellyAuthorizedCfg("/smoke.cfg", out, capacity);
}

bool P4ShellySmokeConfigStore::saveAuthorized(const shelly::ShellyDeviceIdentity* entries,
                                              size_t count) {
  return saveShellyAuthorizedCfg("/smoke.cfg", entries, count);
}

}  // namespace wattcycle::panel
