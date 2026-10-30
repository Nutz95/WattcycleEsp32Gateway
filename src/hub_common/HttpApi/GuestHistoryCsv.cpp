#include "HttpApi/GuestHistoryCsv.h"

#include "Auth/IGuestAccess.h"
#include "Storage/HistoryCsvColumn.h"

#include <cstring>

namespace wattcycle::web {
namespace {

/// Guest tab bits that may see this column. `kAlways` keeps the field for every guest.
constexpr uint32_t kAlways = 0xffffffffu;

constexpr uint32_t kGuestTabsByColumn[static_cast<size_t>(storage::HistoryCsvColumn::Count)] = {
    kAlways,                                                          // Epoch
    auth::IGuestAccess::kTabWattcycle,                                // Soc
    auth::IGuestAccess::kTabWattcycle,                                // PackV
    auth::IGuestAccess::kTabWattcycle,                                // PackA
    auth::IGuestAccess::kTabWattcycle,                                // PackW
    auth::IGuestAccess::kTabSolar,                                    // SolarV
    auth::IGuestAccess::kTabSolar,                                    // SolarA
    auth::IGuestAccess::kTabSolar,                                    // SolarW
    auth::IGuestAccess::kTabWattcycle,                                // PackOk
    auth::IGuestAccess::kTabSolar,                                    // SolarOk
    auth::IGuestAccess::kTabWattcycle,                                // PackAh
    auth::IGuestAccess::kTabWattcycle | auth::IGuestAccess::kTabTemperatures,  // MosC
    auth::IGuestAccess::kTabWattcycle | auth::IGuestAccess::kTabTemperatures,  // PcbC
    auth::IGuestAccess::kTabSolar,                                    // SolarWh
    auth::IGuestAccess::kTabSolar,                                    // SolarAh
    auth::IGuestAccess::kTabEcoFlow,                                  // EcoOk
    auth::IGuestAccess::kTabEcoFlow,                                  // EcoSoc
    auth::IGuestAccess::kTabEcoFlow,                                  // EcoAcOut
    auth::IGuestAccess::kTabEcoFlow,                                  // EcoAcIn
    auth::IGuestAccess::kTabEcoFlow,                                  // EcoDc
    auth::IGuestAccess::kTabEcoFlow,                                  // EcoUsb
};

bool keepHistoryColumn(uint32_t mask, int index) {
  storage::HistoryCsvColumn column = storage::HistoryCsvColumn::Epoch;
  if (!storage::historyCsvColumnAt(index, column)) {
    return false;
  }
  const uint32_t tabs = kGuestTabsByColumn[static_cast<size_t>(column)];
  if (tabs == kAlways) {
    return true;
  }
  return (mask & tabs) != 0;
}

bool emitFilteredLine(GuestHistoryCsv* ctx, char* line) {
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
    const char* value = keep && fields[index] != nullptr ? fields[index] : "";
    const size_t len = std::strlen(value);
    if (pos + len >= sizeof(rebuilt)) {
      break;
    }
    std::memcpy(rebuilt + pos, value, len);
    pos += len;
  }
  rebuilt[pos++] = '\n';
  if (ctx->collect != nullptr) {
    if (!ctx->collect->append(reinterpret_cast<const uint8_t*>(rebuilt), pos)) {
      ctx->collectOk = false;
      return false;
    }
    return true;
  }
  return ctx->server->client().write(reinterpret_cast<const uint8_t*>(rebuilt), pos) == pos;
}

}  // namespace

bool beginGuestHistoryCsv(size_t /*totalBytes*/, void* user) {
  auto* ctx = static_cast<GuestHistoryCsv*>(user);
  if (ctx == nullptr) {
    return false;
  }
  ctx->headerSent = true;
  if (ctx->collect != nullptr) {
    return true;
  }
  if (ctx->server == nullptr) {
    return false;
  }
  ctx->server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  ctx->server->send(200, "text/csv; charset=utf-8", "");
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
    if (ctx->pending[0] != '\0' && !emitFilteredLine(ctx, ctx->pending)) {
      return false;
    }
  }
  return true;
}

}  // namespace wattcycle::web
