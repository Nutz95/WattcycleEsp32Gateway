#pragma once

#include "Shelly/ShellyPlugLimits.h"

#include <cstddef>
#include <cstdint>

namespace wattcycle::shelly {

constexpr size_t kMaxShellySmoke = 4;
/// Plus Smoke sleeps for long stretches; treat as online within two days.
constexpr uint32_t kShellySmokeStaleMs = 48u * 3600u * 1000u;
constexpr uint32_t kShellySmokePostingTtlMs = 10u * 60u * 1000u;

}  // namespace wattcycle::shelly
