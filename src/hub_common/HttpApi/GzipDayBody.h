#pragma once

#include "HttpApi/DayCsvBuffer.h"

#include <cstddef>
#include <cstdint>

#include <WebServer.h>

namespace wattcycle::web {

/// Gzip (RFC 1952) helpers for history day downloads.
class GzipDayBody {
 public:
  /// Compress `plain` into a new heap block. Caller frees with `free`.
  static bool compress(const uint8_t* plain, size_t plainLen, uint8_t** out, size_t* outLen);

  /// Inflate a gzip member produced by `compress` and compare to `plain` (round-trip).
  static bool verify(const uint8_t* plain, size_t plainLen, const uint8_t* gzip, size_t gzipLen);

  /// Weak ETag from byte length (enough to invalidate when the day file grows).
  static void formatEtag(size_t plainLen, char* out, size_t capacity);

  /// True when the client asked for gzip (default) rather than identity CSV.
  static bool wantsGzip(WebServer& server);

  /// Send gzip when valid; otherwise plain CSV. Sets ETag; honors If-None-Match with 304.
  static bool send(WebServer& server, const uint8_t* plain, size_t plainLen, bool preferGzip);

  /// HistoryStreamBegin: reserve from totalBytes when known.
  static bool beginCollect(size_t totalBytes, void* user);
  /// HistoryChunkSink: append into DayCsvBuffer*.
  static bool writeCollect(const uint8_t* data, size_t length, void* user);
};

}  // namespace wattcycle::web
