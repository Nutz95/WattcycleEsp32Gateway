#include "HttpApi/GuestHistoryCsv.h"

#include "Auth/IGuestAccess.h"

#include <cstring>

namespace wattcycle::web {
namespace {

bool keepHistoryColumn(uint32_t mask, int index) {
  if (index <= 0) {
    return true;
  }
  const bool pack = (mask & auth::IGuestAccess::kTabWattcycle) != 0;
  const bool temps = (mask & auth::IGuestAccess::kTabTemperatures) != 0;
  const bool solar = (mask & auth::IGuestAccess::kTabSolar) != 0;
  const bool eco = (mask & auth::IGuestAccess::kTabEcoFlow) != 0;
  if (index == 11 || index == 12) {
    return pack || temps;
  }
  if (index <= 4 || index == 8 || index == 10) {
    return pack;
  }
  if (index <= 7 || index == 9 || index == 13 || index == 14) {
    return solar;
  }
  if (index >= 15 && index <= 20) {
    return eco;
  }
  return false;
}

bool writeFilteredLine(GuestHistoryCsv* ctx, char* line) {
  const bool header = !ctx->sawHeader;
  ctx->sawHeader = true;
  char* fields[24] = {};
  int count = 0;
  fields[0] = line;
  for (char* cursor = line; *cursor != '\0' && count < 23; ++cursor) {
    if (*cursor == ',') {
      *cursor = '\0';
      count += 1;
      fields[count] = cursor + 1;
    }
  }
  count += 1;
  char rebuilt[192] = {};
  size_t pos = 0;
  for (int index = 0; index < count && pos + 2 < sizeof(rebuilt); ++index) {
    if (index > 0) {
      rebuilt[pos++] = ',';
    }
    const bool keep = header || keepHistoryColumn(ctx->mask, index);
    const char* value = keep && fields[index] != nullptr ? fields[index] : "0";
    const size_t len = std::strlen(value);
    if (pos + len >= sizeof(rebuilt)) {
      break;
    }
    std::memcpy(rebuilt + pos, value, len);
    pos += len;
  }
  rebuilt[pos++] = '\n';
  return ctx->server->client().write(reinterpret_cast<const uint8_t*>(rebuilt), pos) == pos;
}

}  // namespace

bool beginGuestHistoryCsv(size_t /*totalBytes*/, void* user) {
  auto* ctx = static_cast<GuestHistoryCsv*>(user);
  if (ctx == nullptr || ctx->server == nullptr) {
    return false;
  }
  ctx->server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  ctx->server->send(200, "text/csv", "");
  ctx->headerSent = true;
  return true;
}

bool writeGuestHistoryChunk(const uint8_t* data, size_t length, void* user) {
  auto* ctx = static_cast<GuestHistoryCsv*>(user);
  if (ctx == nullptr || data == nullptr) {
    return false;
  }
  for (size_t index = 0; index < length; ++index) {
    const char next = static_cast<char>(data[index]);
    if (next == '\r') {
      continue;
    }
    if (next != '\n') {
      if (ctx->used + 1 < sizeof(ctx->pending)) {
        ctx->pending[ctx->used++] = next;
      }
      continue;
    }
    ctx->pending[ctx->used] = '\0';
    ctx->used = 0;
    if (ctx->pending[0] != '\0' && !writeFilteredLine(ctx, ctx->pending)) {
      return false;
    }
  }
  return true;
}

}  // namespace wattcycle::web
