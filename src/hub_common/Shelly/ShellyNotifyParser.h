#pragma once

#include "Shelly/ShellyNotifyDelta.h"

namespace wattcycle::shelly {

/// Pure JSON-RPC NotifyStatus / NotifyFullStatus parser for Shelly Gen2+ UDP frames.
class ShellyNotifyParser {
 public:
  /// Parse one datagram. False leaves `out.ok` false.
  static bool parse(const char* json, ShellyNotifyDelta& out);
};

}  // namespace wattcycle::shelly
