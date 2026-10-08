#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::util {

/// Cryptographic-quality random bytes (esp_random on device, rand in native tests).
void fillRandom(uint8_t* buffer, size_t length);

}  // namespace wattcycle::util
