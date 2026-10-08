#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::auth {

/// Portable SHA-256 (FIPS 180-4) for firmware + native unit tests.
class Sha256 {
 public:
  static void hash(const uint8_t* data, size_t length, uint8_t out[32]);
};

}  // namespace wattcycle::auth
