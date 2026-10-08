#pragma once

#include <cstdio>

namespace wattcycle::util {

/// Portable bounded C-string copy (always NUL-terminates). Prefer over strncpy / strncpy_s.
inline void copyCString(char* dest, size_t destCapacity, const char* src) {
  if (dest == nullptr || destCapacity == 0) {
    return;
  }
  if (src == nullptr) {
    dest[0] = '\0';
    return;
  }
  std::snprintf(dest, destCapacity, "%s", src);
}

}  // namespace wattcycle::util

// XT369P bridge TUs resolve `util::` under namespace xt369p::*.
namespace xt369p::util {
using wattcycle::util::copyCString;
}
