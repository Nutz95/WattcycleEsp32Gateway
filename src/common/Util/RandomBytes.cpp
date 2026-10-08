#include "Util/RandomBytes.h"

#ifndef UNIT_TEST
#include <esp_random.h>
#else
#include <cstdlib>
#endif

namespace wattcycle::util {

void fillRandom(uint8_t* buffer, size_t length) {
  if (buffer == nullptr) {
    return;
  }
  for (size_t i = 0; i < length; ++i) {
#ifndef UNIT_TEST
    buffer[i] = static_cast<uint8_t>(esp_random() & 0xFF);
#else
    buffer[i] = static_cast<uint8_t>(std::rand() & 0xFF);
#endif
  }
}

}  // namespace wattcycle::util
