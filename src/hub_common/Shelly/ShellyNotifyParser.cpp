#include "Shelly/ShellyNotifyParser.h"

#include "HttpApi/JsonField.h"
#include "Shelly/ShellyPlugId.h"

#include <cstdlib>
#include <cstring>

namespace wattcycle::shelly {
namespace {

const char* findAenergyTotal(const char* json) {
  const char* block = std::strstr(json, "\"aenergy\"");
  if (block == nullptr) {
    return nullptr;
  }
  return std::strstr(block, "\"total\"");
}

bool extractNestedTotal(const char* json, float& out) {
  const char* key = findAenergyTotal(json);
  if (key == nullptr) {
    return false;
  }
  const char* cursor = key + 7;
  while (*cursor == ' ' || *cursor == '\t' || *cursor == ':' || *cursor == '\n' || *cursor == '\r') {
    ++cursor;
  }
  char* end = nullptr;
  const double parsed = std::strtod(cursor, &end);
  if (end == cursor) {
    return false;
  }
  out = static_cast<float>(parsed);
  return true;
}

}  // namespace

bool ShellyNotifyParser::parse(const char* json, ShellyNotifyDelta& out) {
  out = {};
  if (json == nullptr || json[0] != '{') {
    return false;
  }
  char method[32] = {};
  if (!web::extractJsonStringField(json, "method", method, sizeof(method))) {
    return false;
  }
  if (std::strcmp(method, "NotifyStatus") == 0) {
    out.full = false;
  } else if (std::strcmp(method, "NotifyFullStatus") == 0) {
    out.full = true;
  } else {
    return false;
  }
  if (!web::extractJsonStringField(json, "src", out.id, sizeof(out.id)) ||
      !shellyPlugIdValid(out.id)) {
    return false;
  }
  float value = 0;
  if (web::extractJsonFloatField(json, "apower", value)) {
    out.hasApower = true;
    out.apower = value;
  }
  if (web::extractJsonFloatField(json, "voltage", value)) {
    out.hasVoltage = true;
    out.voltage = value;
  }
  if (web::extractJsonFloatField(json, "current", value)) {
    out.hasCurrent = true;
    out.current = value;
  }
  if (extractNestedTotal(json, value)) {
    out.hasAenergy = true;
    out.aenergyWh = value;
  }
  bool relay = false;
  if (web::extractJsonBoolField(json, "output", relay)) {
    out.hasOutput = true;
    out.output = relay;
  }
  out.ok = true;
  return true;
}

}  // namespace wattcycle::shelly
