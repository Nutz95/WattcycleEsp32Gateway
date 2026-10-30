#pragma once

#include "HttpApi/DayCsvBuffer.h"

#include <WebServer.h>

#include <cstddef>
#include <cstdint>

namespace wattcycle::web {

/// Line buffer for a guest history download. Columns the guest cannot see become empty.
struct GuestHistoryCsv {
  WebServer* server = nullptr;
  DayCsvBuffer* collect = nullptr;
  uint32_t mask = 0;
  char pending[192] = {};
  size_t used = 0;
  bool headerSent = false;
  bool sawHeader = false;
  bool collectOk = true;
};

/// Begin callback: marks the day as found (streams headers when not collecting).
bool beginGuestHistoryCsv(size_t totalBytes, void* user);
/// Chunk callback: filter columns, then stream or append to `collect`.
bool writeGuestHistoryChunk(const uint8_t* data, size_t length, void* user);

}  // namespace wattcycle::web
