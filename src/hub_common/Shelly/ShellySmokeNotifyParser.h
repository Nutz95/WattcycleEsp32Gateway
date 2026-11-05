#pragma once

#include "Shelly/ShellySmokeNotifyDelta.h"

namespace wattcycle::shelly {

/// Parses Shelly Plus Smoke UDP RPC NotifyStatus frames.
class ShellySmokeNotifyParser {
 public:
  static bool parse(const char* json, ShellySmokeNotifyDelta& out);
};

}  // namespace wattcycle::shelly
