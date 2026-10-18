#pragma once

#include <cstddef>
#include <cstdint>

namespace ecoflow::crypto {

/// Public EcoFlow app login-key table (ef-ble-reverse); embedded at build from
/// third_party/ecoflow/login_key.bin. Generated .cpp is gitignored.
extern const uint8_t kEcoFlowLoginKey[];
extern const size_t kEcoFlowLoginKeySize;

}  // namespace ecoflow::crypto
