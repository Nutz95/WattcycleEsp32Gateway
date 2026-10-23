#pragma once

#include <WebServer.h>

#include <cstddef>
#include <cstdint>

namespace wattcycle::web {

/// Line buffer for a guest history download. Columns the guest cannot see become 0.
struct GuestHistoryCsv {
  WebServer* server = nullptr;
  uint32_t mask = 0;
  char pending[192] = {};
  size_t used = 0;
  bool headerSent = false;
  bool sawHeader = false;
};

/// HTTP begin callback for HistoryStreamBegin.
bool beginGuestHistoryCsv(size_t totalBytes, void* user);
/// HTTP chunk callback for HistoryChunkSink.
bool writeGuestHistoryChunk(const uint8_t* data, size_t length, void* user);

}  // namespace wattcycle::web
