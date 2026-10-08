#pragma once

#include <cstddef>

namespace wattcycle::web {

/// Minimal JSON string field extractor for small auth payloads (no ArduinoJson).
bool extractJsonStringField(const char* json, const char* key, char* out, size_t capacity);

}  // namespace wattcycle::web
