#include "Shelly/ShellyDisplayName.h"

#include "Util/SafeCopy.h"

#include <cstring>

namespace wattcycle::shelly {

bool ShellyDisplayName::sanitize(const char* name, char* out, size_t capacity) {
  if (out == nullptr || capacity == 0) {
    return false;
  }
  out[0] = '\0';
  if (name == nullptr) {
    return false;
  }
  size_t written = 0;
  for (const char* cursor = name; *cursor != '\0' && written + 1 < capacity; ++cursor) {
    const unsigned char ch = static_cast<unsigned char>(*cursor);
    if (ch < 32u || ch == 127u || ch == '|' || ch == '\n' || ch == '\r' || ch == '"' ||
        ch == '\\' || ch == ',') {
      continue;
    }
    out[written++] = static_cast<char>(ch);
  }
  out[written] = '\0';
  return written > 0;
}

bool ShellyDisplayName::shortFromId(const char* id, char* out, size_t capacity) {
  if (id == nullptr) {
    return false;
  }
  const char* dash = std::strrchr(id, '-');
  const char* tail = dash != nullptr && dash[1] != '\0' ? dash + 1 : id;
  util::copyCString(out, capacity, tail);
  return out[0] != '\0';
}

}  // namespace wattcycle::shelly
