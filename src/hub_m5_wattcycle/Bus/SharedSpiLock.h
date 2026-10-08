#pragma once

#include <cstdint>

namespace wattcycle::bus {

/// Serializes M5 LCD blits and SD card I/O on the shared VSPI bus.
class SharedSpiLock {
 public:
  static void begin();
  static void lock();
  static void unlock();
};

struct SpiGuard {
  SpiGuard() { SharedSpiLock::lock(); }
  ~SpiGuard() { SharedSpiLock::unlock(); }
  SpiGuard(const SpiGuard&) = delete;
  SpiGuard& operator=(const SpiGuard&) = delete;
};

}  // namespace wattcycle::bus
