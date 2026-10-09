#include "HttpApi/EspWebGateway.h"

#ifndef UNIT_TEST

#include <cstdio>
#include <cstring>

namespace wattcycle::web {
namespace {

struct DayStreamContext {
  WebServer* server = nullptr;
  bool headerSent = false;
  bool aborted = false;
};

bool beginDayStream(size_t totalBytes, void* user) {
  auto* ctx = static_cast<DayStreamContext*>(user);
  if (ctx == nullptr || ctx->server == nullptr) {
    return false;
  }
  if (totalBytes > 0) {
    ctx->server->setContentLength(totalBytes);
  } else {
    ctx->server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  }
  ctx->server->send(200, "text/csv", "");
  ctx->headerSent = true;
  return true;
}

bool writeDayChunk(const uint8_t* data, size_t length, void* user) {
  auto* ctx = static_cast<DayStreamContext*>(user);
  if (ctx == nullptr || ctx->server == nullptr || data == nullptr) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  const size_t written = ctx->server->client().write(data, length);
  if (written != length) {
    ctx->aborted = true;
    return false;
  }
  return true;
}

}  // namespace

void EspWebGateway::handleHistoryDays() {
  if (!requireAuth()) {
    return;
  }
  const bool ntp = store_.status().ntpSynced;
  const bool sd = historyStore_.isReady();
  char json[768] = {};
  size_t pos = 0;
  pos += static_cast<size_t>(
      std::snprintf(json + pos, sizeof(json) - pos, "{\"ntp\":%s,\"sd\":%s,\"days\":[",
                    ntp ? "true" : "false", sd ? "true" : "false"));
  if (sd) {
    char days[16][11] = {};
    const size_t count = historyStore_.listDays(days, 16);
    for (size_t i = 0; i < count && pos + 16 < sizeof(json); ++i) {
      pos += static_cast<size_t>(
          std::snprintf(json + pos, sizeof(json) - pos, "%s\"%s\"", i == 0 ? "" : ",", days[i]));
    }
  }
  if (pos + 3 < sizeof(json)) {
    json[pos++] = ']';
    json[pos++] = '}';
    json[pos] = '\0';
  }
  sendJson(200, json);
}

void EspWebGateway::handleHistoryDay() {
  if (!requireAuth()) {
    return;
  }
  if (!historyStore_.isReady()) {
    sendJson(503, "{\"error\":\"sd_unavailable\"}");
    return;
  }
  if (!server_->hasArg("date")) {
    sendJson(400, "{\"error\":\"missing_date\"}");
    return;
  }
  const String date = server_->arg("date");
  DayStreamContext ctx;
  ctx.server = server_.get();
  if (!historyStore_.streamDay(date.c_str(), &beginDayStream, &writeDayChunk, &ctx) ||
      ctx.aborted) {
    if (!ctx.headerSent) {
      sendJson(404, "{\"error\":\"day_not_found\"}");
    }
    return;
  }
  if (!ctx.headerSent) {
    sendJson(404, "{\"error\":\"day_not_found\"}");
  }
}

void EspWebGateway::handleDeviceInfo() {
  if (!requireAuth()) {
    return;
  }
  const char* role = role_[0] != '\0' ? role_ : "hub";
  char json[448] = {};
  std::snprintf(json, sizeof(json),
                "{\"role\":\"%s\",\"ntp\":%s,\"sd\":%s,"
                "\"hubStaMac\":\"%s\",\"bmsBridgeMac\":\"%s\",\"xtBridgeMac\":\"%s\","
                "\"ecoflowBridgeMac\":\"%s\"}",
                role, store_.status().ntpSynced ? "true" : "false",
                historyStore_.isReady() ? "true" : "false", hubStaMac_, bmsBridgeMac_,
                xtBridgeMac_, ecoflowBridgeMac_);
  sendJson(200, json);
}

}  // namespace wattcycle::web

#endif
