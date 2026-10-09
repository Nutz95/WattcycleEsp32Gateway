#pragma once

#include <cstdint>

namespace wattcycle::util {

/// Parse `AA:BB:CC:DD:EE:FF` or `AA-BB-...` (case-insensitive). Rejects trailing junk.
inline int hexNibble(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return 10 + (c - 'a');
  }
  if (c >= 'A' && c <= 'F') {
    return 10 + (c - 'A');
  }
  return -1;
}

inline bool parseMacAddress(const char* text, uint8_t out[6]) {
  if (text == nullptr || text[0] == '\0' || out == nullptr) {
    return false;
  }
  const char* cursor = text;
  for (int i = 0; i < 6; ++i) {
    const int hi = hexNibble(cursor[0]);
    const int lo = hexNibble(cursor[1]);
    if (hi < 0 || lo < 0) {
      return false;
    }
    out[i] = static_cast<uint8_t>((hi << 4) | lo);
    cursor += 2;
    if (i < 5) {
      if (*cursor != ':' && *cursor != '-') {
        return false;
      }
      ++cursor;
    }
  }
  return *cursor == '\0';
}

}  // namespace wattcycle::util

// Bridge TUs resolve `util::` under their root namespace.
namespace xt369p::util {
using wattcycle::util::hexNibble;
using wattcycle::util::parseMacAddress;
}

namespace ecoflow::util {
using wattcycle::util::hexNibble;
using wattcycle::util::parseMacAddress;
}
