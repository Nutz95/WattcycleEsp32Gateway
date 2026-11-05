#include "Shelly/ShellySmokeNotifyParser.h"

#include "HttpApi/JsonField.h"
#include "Shelly/ShellySmokeId.h"

#include <cstring>

namespace wattcycle::shelly {

bool ShellySmokeNotifyParser::parse(const char* json, ShellySmokeNotifyDelta& out) {
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
      !shellySmokeIdValid(out.id)) {
    return false;
  }
  bool flag = false;
  if (web::extractJsonBoolField(json, "alarm", flag)) {
    out.hasAlarm = true;
    out.alarm = flag;
  }
  if (web::extractJsonBoolField(json, "mute", flag)) {
    out.hasMute = true;
    out.mute = flag;
  }
  float value = 0;
  if (web::extractJsonFloatField(json, "percent", value)) {
    out.hasBatteryPercent = true;
    out.batteryPercent = value;
  }
  if (web::extractJsonFloatField(json, "V", value)) {
    out.hasBatteryVolts = true;
    out.batteryVolts = value;
  }
  out.ok = true;
  return true;
}

}  // namespace wattcycle::shelly
