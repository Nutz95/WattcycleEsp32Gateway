#pragma once

#include <cstddef>

namespace wattcycle::shelly {

/// Shared display-name helpers for Shelly whitelist entries.
class ShellyDisplayName {
 public:
  /// Strip control and delimiter chars into `out`. False when empty.
  static bool sanitize(const char* name, char* out, size_t capacity);
  /// Use the hex tail after the last '-' in a Shelly id.
  static bool shortFromId(const char* id, char* out, size_t capacity);
};

}  // namespace wattcycle::shelly
