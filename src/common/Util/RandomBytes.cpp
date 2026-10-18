#include "Util/RandomBytes.h"

#if __has_include(<esp_random.h>)
#include <esp_random.h>
#else
#include <cstdlib>
#endif

namespace wattcycle::util {
namespace {

uint8_t nextRandomByte() {
#if __has_include(<esp_random.h>)
  return static_cast<uint8_t>(esp_random() & 0xFF);
#else
  return static_cast<uint8_t>(std::rand() & 0xFF);
#endif
}

}  // namespace

void fillRandom(uint8_t* buffer, size_t length) {
  if (buffer == nullptr) {
    return;
  }
  for (size_t i = 0; i < length; ++i) {
    buffer[i] = nextRandomByte();
  }
}

}  // namespace wattcycle::util
