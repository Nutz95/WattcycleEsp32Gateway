#pragma once

#include "Shelly/ShellySmokeLimits.h"

#include <cstring>

namespace wattcycle::shelly {

inline bool shellySmokeIdValid(const char* id) {
  return id != nullptr && std::strncmp(id, "shellyplussmoke-", 16) == 0 &&
         std::strlen(id) < kShellyIdCapacity && id[16] != '\0';
}

}  // namespace wattcycle::shelly
