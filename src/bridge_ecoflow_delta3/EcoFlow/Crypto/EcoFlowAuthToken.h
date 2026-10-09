#pragma once

#include <cstddef>
#include <cstdint>

namespace ecoflow::crypto {

/// Builds uppercase hex MD5(userId + serial) used by V3 autoAuthentication.
class EcoFlowAuthToken {
 public:
  static constexpr size_t kDigestBytes = 16;
  static constexpr size_t kHexBytes = 32;  // ASCII hex, no NUL

  /// Writes 32 ASCII hex chars to `outHex32` (not NUL-terminated). Returns false if inputs empty.
  static bool buildUpperHexMd5(const char* userId, const char* serial, char outHex32[kHexBytes]);
};

}  // namespace ecoflow::crypto
