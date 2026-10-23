#pragma once

#include <cstddef>

namespace wattcycle::web {

/// Minimal JSON field extractors for small gateway payloads (no ArduinoJson).
bool extractJsonStringField(const char* json, const char* key, char* out, size_t capacity);
/// True when `key` is present as a JSON true or false literal.
bool extractJsonBoolField(const char* json, const char* key, bool& out);
/// True when `key` is present as a JSON integer.
bool extractJsonIntField(const char* json, const char* key, int& out);
/// True when `key` is present as a JSON number (int or float).
bool extractJsonFloatField(const char* json, const char* key, float& out);

}  // namespace wattcycle::web
