#pragma once

#include <cstddef>
#include <cstdint>

namespace ecoflow::crypto {

/// Public EcoFlow app login-key table (from reverse-eng); embedded at build from
/// secrets/ecoflow/login_key.bin — never commit the bin or generated .cpp.
extern const uint8_t kEcoFlowLoginKey[];
extern const size_t kEcoFlowLoginKeySize;

}  // namespace ecoflow::crypto
