#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::shelly {

constexpr size_t kMaxShellyPlugs = 8;
constexpr size_t kShellyIdCapacity = 48;
constexpr size_t kShellyNameCapacity = 25;
constexpr uint16_t kShellyUdpPort = 2267;
constexpr uint32_t kShellyOfflineMs = 60u * 1000u;
constexpr uint32_t kShellyPostingTtlMs = 5u * 60u * 1000u;
constexpr uint32_t kShellyListenMs = 2u * 60u * 1000u;
constexpr size_t kShellyUdpBuffer = 1500;

}  // namespace wattcycle::shelly
