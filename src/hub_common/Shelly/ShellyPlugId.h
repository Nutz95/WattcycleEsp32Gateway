#pragma once

#include "Shelly/ShellyPlugLimits.h"

#include <cstring>

namespace wattcycle::shelly {

inline bool shellyPlugIdValid(const char* id) {
  return id != nullptr && std::strncmp(id, "shellyplugmg3-", 14) == 0 &&
         std::strlen(id) < kShellyIdCapacity && id[14] != '\0';
}

}  // namespace wattcycle::shelly
