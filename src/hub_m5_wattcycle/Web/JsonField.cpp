#include "Web/JsonField.h"

#include <cstring>

namespace wattcycle::web {

bool extractJsonStringField(const char* json, const char* key, char* out, size_t capacity) {
  if (json == nullptr || key == nullptr || out == nullptr || capacity == 0) {
    return false;
  }
  out[0] = '\0';

  char pattern[48];
  const size_t keyLen = std::strlen(key);
  if (keyLen + 4 >= sizeof(pattern)) {
    return false;
  }
  pattern[0] = '"';
  std::memcpy(pattern + 1, key, keyLen);
  pattern[keyLen + 1] = '"';
  pattern[keyLen + 2] = '\0';

  const char* keyPos = std::strstr(json, pattern);
  if (keyPos == nullptr) {
    return false;
  }
  const char* cursor = keyPos + keyLen + 2;
  while (*cursor == ' ' || *cursor == '\t' || *cursor == ':' || *cursor == '\n' ||
         *cursor == '\r') {
    ++cursor;
  }
  if (*cursor != '"') {
    return false;
  }
  ++cursor;
  size_t written = 0;
  while (*cursor != '\0' && *cursor != '"') {
    if (*cursor == '\\' && cursor[1] != '\0') {
      ++cursor;
    }
    if (written + 1 >= capacity) {
      out[0] = '\0';
      return false;
    }
    out[written++] = *cursor++;
  }
  if (*cursor != '"') {
    out[0] = '\0';
    return false;
  }
  out[written] = '\0';
  return true;
}

}  // namespace wattcycle::web
