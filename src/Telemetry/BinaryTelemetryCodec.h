#pragma once

#include <cstddef>
#include <cstdint>

#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::telemetry {

/// Compact little-endian binary snapshot for the web front-end decoder.
/// Layout version is embedded so the JS decoder can reject mismatches.
class BinaryTelemetryCodec {
 public:
  static constexpr uint32_t kMagic = 0x4D475457u;  // 'WTGM' LE
  static constexpr uint8_t kVersion = 1;
  static constexpr size_t kMaxEncodedBytes = 512;

  static size_t encode(const ITelemetryStore& store, uint8_t* buffer, size_t capacity);
};

}  // namespace wattcycle::telemetry
