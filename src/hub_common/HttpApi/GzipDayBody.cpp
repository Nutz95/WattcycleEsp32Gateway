#include "HttpApi/GzipDayBody.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if __has_include("miniz.h")
#include "miniz.h"
#elif __has_include("esp32/rom/miniz.h")
#include "esp32/rom/miniz.h"
#elif __has_include("esp32s3/rom/miniz.h")
#include "esp32s3/rom/miniz.h"
#elif __has_include("rom/miniz.h")
#include "rom/miniz.h"
#else
#error "miniz.h not found (ESP-IDF ROM miniz required for GzipDayBody)"
#endif

namespace wattcycle::web {
namespace {

constexpr uint8_t kGzipHeader[10] = {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff};

void writeLe32(uint8_t* dest, uint32_t value) {
  dest[0] = static_cast<uint8_t>(value);
  dest[1] = static_cast<uint8_t>(value >> 8);
  dest[2] = static_cast<uint8_t>(value >> 16);
  dest[3] = static_cast<uint8_t>(value >> 24);
}

uint32_t readLe32(const uint8_t* src) {
  return static_cast<uint32_t>(src[0]) | (static_cast<uint32_t>(src[1]) << 8) |
         (static_cast<uint32_t>(src[2]) << 16) | (static_cast<uint32_t>(src[3]) << 24);
}

/// ISO-HDLC CRC32 for gzip footers (not esp_rom_crc32_le / mz_crc32).
uint32_t gzipCrc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xffffffffu;
  for (size_t index = 0; index < length; ++index) {
    crc ^= data[index];
    for (int bit = 0; bit < 8; ++bit) {
      const uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xedb88320u & mask);
    }
  }
  return crc ^ 0xffffffffu;
}

bool deflateTo(const uint8_t* plain, size_t plainLen, uint8_t* dest, size_t destCap, size_t* wrote,
               int flags) {
  if (wrote == nullptr || dest == nullptr) {
    return false;
  }
  *wrote = 0;
  if (plainLen == 0) {
    if (destCap < 2) {
      return false;
    }
    dest[0] = 0x03;
    dest[1] = 0x00;
    *wrote = 2;
    return true;
  }
  const size_t n = tdefl_compress_mem_to_mem(dest, destCap, plain, plainLen, flags);
  if (n == 0) {
    return false;
  }
  *wrote = n;
  return true;
}

template <typename StreamT>
bool writeAll(StreamT& client, const uint8_t* data, size_t length) {
  size_t sent = 0;
  while (sent < length) {
    const size_t chunk = length - sent > 1436 ? 1436 : length - sent;
    const size_t n = client.write(data + sent, chunk);
    if (n == 0) {
      return false;
    }
    sent += n;
  }
  return true;
}

}  // namespace

bool GzipDayBody::compress(const uint8_t* plain, size_t plainLen, uint8_t** out, size_t* outLen) {
  if (out == nullptr || outLen == nullptr) {
    return false;
  }
  *out = nullptr;
  *outLen = 0;
  if (plain == nullptr && plainLen != 0) {
    return false;
  }
  // Worst-case raw blocks + gzip wrapper headroom.
  const size_t bound = plainLen + (plainLen / 8) + 512;
  uint8_t* blob = static_cast<uint8_t*>(std::malloc(bound));
  if (blob == nullptr) {
    return false;
  }
  std::memcpy(blob, kGzipHeader, sizeof(kGzipHeader));
  size_t used = sizeof(kGzipHeader);
  size_t deflateWrote = 0;
  const size_t deflateCap = bound - used - 8;
  const int flags = static_cast<int>(TDEFL_DEFAULT_MAX_PROBES);
  if (!deflateTo(plain, plainLen, blob + used, deflateCap, &deflateWrote, flags)) {
    // Guaranteed-valid fallback: stored/raw deflate blocks (larger, always gunzippable).
    if (!deflateTo(plain, plainLen, blob + used, deflateCap, &deflateWrote,
                   static_cast<int>(TDEFL_FORCE_ALL_RAW_BLOCKS))) {
      std::free(blob);
      return false;
    }
  }
  used += deflateWrote;
  const uint32_t crc = gzipCrc32(plain == nullptr ? reinterpret_cast<const uint8_t*>("") : plain,
                                 plainLen);
  writeLe32(blob + used, crc);
  used += 4;
  writeLe32(blob + used, static_cast<uint32_t>(plainLen));
  used += 4;
  *out = blob;
  *outLen = used;
  return true;
}

bool GzipDayBody::verify(const uint8_t* plain, size_t plainLen, const uint8_t* gzip, size_t gzipLen) {
  if (gzip == nullptr || gzipLen < sizeof(kGzipHeader) + 8) {
    return false;
  }
  if (std::memcmp(gzip, kGzipHeader, 3) != 0) {
    return false;
  }
  const size_t deflateLen = gzipLen - sizeof(kGzipHeader) - 8;
  const uint8_t* deflate = gzip + sizeof(kGzipHeader);
  const uint32_t expectCrc = readLe32(gzip + gzipLen - 8);
  const uint32_t expectSize = readLe32(gzip + gzipLen - 4);
  if (expectSize != static_cast<uint32_t>(plainLen)) {
    return false;
  }
  if (plainLen == 0) {
    return expectCrc == gzipCrc32(nullptr, 0) || expectCrc == gzipCrc32(reinterpret_cast<const uint8_t*>(""), 0);
  }
  uint8_t* round = static_cast<uint8_t*>(std::malloc(plainLen ? plainLen : 1));
  if (round == nullptr) {
    return false;
  }
  const size_t got =
      tinfl_decompress_mem_to_mem(round, plainLen, deflate, deflateLen, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
  if (got != plainLen) {
    std::free(round);
    return false;
  }
  const bool same = std::memcmp(round, plain, plainLen) == 0;
  const uint32_t crc = gzipCrc32(round, plainLen);
  std::free(round);
  return same && crc == expectCrc;
}

void GzipDayBody::formatEtag(size_t plainLen, char* out, size_t capacity) {
  if (out == nullptr || capacity < 12) {
    return;
  }
  std::snprintf(out, capacity, "\"%08lx\"", static_cast<unsigned long>(plainLen));
}

bool GzipDayBody::wantsGzip(WebServer& server) {
  if (!server.hasArg("encoding")) {
    return true;
  }
  const String value = server.arg("encoding");
  return value != "identity" && value != "csv";
}

bool GzipDayBody::beginCollect(size_t totalBytes, void* user) {
  auto* buffer = static_cast<DayCsvBuffer*>(user);
  if (buffer == nullptr) {
    return false;
  }
  if (totalBytes > 0 && !buffer->reserve(totalBytes)) {
    return false;
  }
  return true;
}

bool GzipDayBody::writeCollect(const uint8_t* data, size_t length, void* user) {
  auto* buffer = static_cast<DayCsvBuffer*>(user);
  if (buffer == nullptr) {
    return false;
  }
  return buffer->append(data, length);
}

bool GzipDayBody::send(WebServer& server, const uint8_t* plain, size_t plainLen, bool preferGzip) {
  char etag[16] = {};
  formatEtag(plainLen, etag, sizeof(etag));
  if (server.hasHeader("If-None-Match") && server.header("If-None-Match") == String(etag)) {
    server.send(304, preferGzip ? "application/gzip" : "text/csv; charset=utf-8", "");
    return true;
  }

  uint8_t* compressed = nullptr;
  size_t compressedLen = 0;
  bool useGzip = preferGzip;
  if (useGzip) {
    if (!compress(plain, plainLen, &compressed, &compressedLen) ||
        !verify(plain, plainLen, compressed, compressedLen)) {
      std::free(compressed);
      compressed = nullptr;
      compressedLen = 0;
      useGzip = false;
    }
  }

  server.sendHeader("ETag", etag);
  server.sendHeader("Cache-Control", "private, max-age=0");
  if (!useGzip) {
    server.setContentLength(plainLen);
    server.send(200, "text/csv; charset=utf-8", "");
    if (plainLen > 0 && plain != nullptr) {
      auto client = server.client();
      writeAll(client, plain, plainLen);
    }
    return true;
  }
  server.setContentLength(compressedLen);
  server.send(200, "application/gzip", "");
  {
    auto client = server.client();
    writeAll(client, compressed, compressedLen);
  }
  std::free(compressed);
  return true;
}

}  // namespace wattcycle::web
