#include "HttpApi/JsonField.h"

#include <cstdlib>
#include <cstring>

namespace wattcycle::web {
namespace {

const char* valueAfterKey(const char* json, const char* key) {
  if (json == nullptr || key == nullptr) {
    return nullptr;
  }
  char pattern[48];
  const size_t keyLen = std::strlen(key);
  if (keyLen + 4 >= sizeof(pattern)) {
    return nullptr;
  }
  pattern[0] = '"';
  std::memcpy(pattern + 1, key, keyLen);
  pattern[keyLen + 1] = '"';
  pattern[keyLen + 2] = '\0';
  const char* keyPos = std::strstr(json, pattern);
  if (keyPos == nullptr) {
    return nullptr;
  }
  const char* cursor = keyPos + keyLen + 2;
  while (*cursor == ' ' || *cursor == '\t' || *cursor == ':' || *cursor == '\n' || *cursor == '\r') {
    ++cursor;
  }
  return cursor;
}

}  // namespace

bool extractJsonStringField(const char* json, const char* key, char* out, size_t capacity) {
  if (out == nullptr || capacity == 0) {
    return false;
  }
  out[0] = '\0';
  const char* cursor = valueAfterKey(json, key);
  if (cursor == nullptr || *cursor != '"') {
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

bool extractJsonBoolField(const char* json, const char* key, bool& out) {
  const char* cursor = valueAfterKey(json, key);
  if (cursor == nullptr) {
    return false;
  }
  if (std::strncmp(cursor, "true", 4) == 0) {
    out = true;
    return true;
  }
  if (std::strncmp(cursor, "false", 5) == 0) {
    out = false;
    return true;
  }
  return false;
}

bool extractJsonIntField(const char* json, const char* key, int& out) {
  const char* cursor = valueAfterKey(json, key);
  if (cursor == nullptr) {
    return false;
  }
  char* end = nullptr;
  const long parsed = std::strtol(cursor, &end, 10);
  if (end == cursor) {
    return false;
  }
  out = static_cast<int>(parsed);
  return true;
}

bool extractJsonFloatField(const char* json, const char* key, float& out) {
  const char* cursor = valueAfterKey(json, key);
  if (cursor == nullptr) {
    return false;
  }
  char* end = nullptr;
  const double parsed = std::strtod(cursor, &end);
  if (end == cursor) {
    return false;
  }
  out = static_cast<float>(parsed);
  return true;
}

}  // namespace wattcycle::web
