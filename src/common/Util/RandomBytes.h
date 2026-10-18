#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::util {

/// Fill buffer with random bytes (esp_random on ESP, rand on host).
void fillRandom(uint8_t* buffer, size_t length);

}  // namespace wattcycle::util
