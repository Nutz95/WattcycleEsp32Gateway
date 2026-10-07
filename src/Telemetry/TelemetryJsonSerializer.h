#pragma once

#include <cstddef>

#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::telemetry {

/// Serializes telemetry snapshots to a compact JSON document.
class TelemetryJsonSerializer {
 public:
  /// Returns bytes written excluding the null terminator, or 0 on failure.
  static size_t serialize(const ITelemetryStore& store, char* buffer, size_t capacity);
};

}  // namespace wattcycle::telemetry
