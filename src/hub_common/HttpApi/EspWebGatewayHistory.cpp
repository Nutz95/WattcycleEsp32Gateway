#include "HttpApi/EspWebGateway.h"

#include "HttpApi/GuestHistoryCsv.h"
#include "HttpApi/HttpReadDecision.h"

#include <ctime>
#include <cstdio>
#include <cstring>

namespace wattcycle::web {
namespace {

struct DayStreamContext {
  WebServer* server = nullptr;
  bool headerSent = false;
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
  return written == length;
}

void currentUtc(int& year, int& month) {
  const std::time_t now = std::time(nullptr);
  struct tm info = {};
  if (gmtime_r(&now, &info) == nullptr || info.tm_year + 1900 < 2024) {
    year = 0;
    month = 0;
    return;
  }
  year = info.tm_year + 1900;
  month = info.tm_mon + 1;
}

int periodYear(const char* period) {
  int year = 0;
  if (period != nullptr) {
    std::sscanf(period, "%d", &year);
  }
  return year;
}

int periodMonth(const char* period) {
  int year = 0;
  int month = 0;
  if (period != nullptr) {
    std::sscanf(period, "%d-%d", &year, &month);
  }
  return month;
}

int newestMonth(const char periods[][8], size_t count, int year) {
  for (size_t i = 0; i < count; ++i) {
    if (periodYear(periods[i]) == year) {
      return periodMonth(periods[i]);
    }
  }
  return 0;
}

void choosePeriod(const char periods[][8], size_t count, int nowYear, int nowMonth, int& year,
                  int& month) {
  if (year <= 0) {
    year = nowYear > 0 ? nowYear : (count > 0 ? periodYear(periods[0]) : 0);
  }
  if (month <= 0) {
    month = (year == nowYear && nowMonth > 0) ? nowMonth : newestMonth(periods, count, year);
  }
}

size_t appendText(char* json, size_t cap, size_t pos, const char* text) {
  if (pos >= cap) {
    return cap;
  }
  const int wrote = std::snprintf(json + pos, cap - pos, "%s", text == nullptr ? "" : text);
  if (wrote < 0) {
    return cap;
  }
  return pos + static_cast<size_t>(wrote);
}

bool hasInt(const int* values, size_t count, int value) {
  for (size_t i = 0; i < count; ++i) {
    if (values[i] == value) {
      return true;
    }
  }
  return false;
}

void pushInt(int* values, size_t& count, size_t maxCount, int value) {
  if (count >= maxCount || hasInt(values, count, value)) {
    return;
  }
  values[count] = value;
  ++count;
}

void collectLists(const char periods[][8], size_t count, int nowYear, int nowMonth, int year,
                  int* years, size_t& yearCount, int* months, size_t& monthCount) {
  yearCount = 0;
  monthCount = 0;
  if (nowYear >= 2024) {
    pushInt(years, yearCount, 48, nowYear);
  }
  if (year == nowYear && nowMonth >= 1 && nowMonth <= 12) {
    pushInt(months, monthCount, 12, nowMonth);
  }
  for (size_t i = 0; i < count; ++i) {
    const int periodY = periodYear(periods[i]);
    const int periodM = periodMonth(periods[i]);
    if (periodY >= 2024) {
      pushInt(years, yearCount, 48, periodY);
    }
    if (periodY == year && periodM >= 1 && periodM <= 12) {
      pushInt(months, monthCount, 12, periodM);
    }
  }
}

size_t appendInts(char* json, size_t cap, size_t pos, const char* key, const int* values,
                  size_t count) {
  pos = appendText(json, cap, pos, key);
  for (size_t i = 0; i < count && pos + 8 < cap; ++i) {
    pos += static_cast<size_t>(
        std::snprintf(json + pos, cap - pos, "%s%d", i == 0 ? "" : ",", values[i]));
  }
  return appendText(json, cap, pos, "]");
}

size_t appendDays(char* json, size_t cap, size_t pos, const char days[][11], size_t count) {
  pos = appendText(json, cap, pos, "\"days\":[");
  for (size_t i = 0; i < count && pos + 16 < cap; ++i) {
    pos += static_cast<size_t>(
        std::snprintf(json + pos, cap - pos, "%s\"%s\"", i == 0 ? "" : ",", days[i]));
  }
  return appendText(json, cap, pos, "]");
}

}  // namespace

void EspWebGateway::handleHistoryDays() {
  char token[auth::kSessionTokenHexLen + 1] = {};
  collectSessionToken(token, sizeof(token));
  if (!decideHttpRead(guest_, authSessions_, token).allows(auth::IGuestAccess::kTabHistory)) {
    sendJson(401, "{\"error\":\"unauthorized\"}");
    return;
  }
  static char periods[48][8];
  static char days[31][11];
  static char json[960];
  const bool sd = historyStore_.isReady();
  const size_t periodCount = sd ? historyStore_.listPeriods(periods, 48) : 0;
  int year = server_->hasArg("year") ? server_->arg("year").toInt() : 0;
  int month = server_->hasArg("month") ? server_->arg("month").toInt() : 0;
  int nowYear = 0;
  int nowMonth = 0;
  currentUtc(nowYear, nowMonth);
  choosePeriod(periods, periodCount, nowYear, nowMonth, year, month);
  const size_t dayCount =
      sd && year >= 2024 ? historyStore_.listMonthDays(year, month, days, 31) : 0;
  int years[48] = {};
  int months[12] = {};
  size_t yearCount = 0;
  size_t monthCount = 0;
  collectLists(periods, periodCount, nowYear, nowMonth, year, years, yearCount, months, monthCount);
  const bool ntp = store_.status().ntpSynced;
  size_t pos = static_cast<size_t>(std::snprintf(
      json, sizeof(json), "{\"ntp\":%s,\"sd\":%s,\"year\":%d,\"month\":%d,",
      ntp ? "true" : "false", sd ? "true" : "false", year, month));
  pos = appendInts(json, sizeof(json), pos, "\"years\":[", years, yearCount);
  pos = appendText(json, sizeof(json), pos, ",");
  pos = appendInts(json, sizeof(json), pos, "\"months\":[", months, monthCount);
  pos = appendText(json, sizeof(json), pos, ",");
  pos = appendDays(json, sizeof(json), pos, days, dayCount);
  pos = appendText(json, sizeof(json), pos, "}");
  if (pos >= sizeof(json)) {
    pos = sizeof(json) - 1;
  }
  json[pos] = '\0';
  sendJson(200, json);
}

void EspWebGateway::handleHistoryDay() {
  char token[auth::kSessionTokenHexLen + 1] = {};
  collectSessionToken(token, sizeof(token));
  const HttpReadDecision access = decideHttpRead(guest_, authSessions_, token);
  if (!access.allows(auth::IGuestAccess::kTabHistory)) {
    sendJson(401, "{\"error\":\"unauthorized\"}");
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
  if (access.guest) {
    GuestHistoryCsv filter;
    filter.server = server_.get();
    filter.mask = access.mask;
    historyStore_.streamDay(date.c_str(), &beginGuestHistoryCsv, &writeGuestHistoryChunk, &filter);
    if (!filter.headerSent) {
      sendJson(404, "{\"error\":\"day_not_found\"}");
    }
    return;
  }
  DayStreamContext ctx;
  ctx.server = server_.get();
  historyStore_.streamDay(date.c_str(), &beginDayStream, &writeDayChunk, &ctx);
  if (!ctx.headerSent) {
    sendJson(404, "{\"error\":\"day_not_found\"}");
  }
}

void EspWebGateway::handleDeviceInfo() {
  char token[auth::kSessionTokenHexLen + 1] = {};
  collectSessionToken(token, sizeof(token));
  const HttpReadDecision access = decideHttpRead(guest_, authSessions_, token);
  const bool identity = access.allows(auth::IGuestAccess::kTabGateway) ||
                        access.allows(auth::IGuestAccess::kTabEsp);
  if (!identity) {
    sendJson(401, "{\"error\":\"unauthorized\"}");
    return;
  }
  const bool guest = access.guest;
  const char* role = role_[0] != '\0' ? role_ : "hub";
  const auto status = store_.status();
  const char* hubMac = guest ? "" : hubStaMac_;
  const char* bmsMac = guest ? "" : bmsBridgeMac_;
  const char* xtMac = guest ? "" : xtBridgeMac_;
  const char* ecoMac = guest ? "" : ecoflowBridgeMac_;
  char json[512] = {};
  std::snprintf(json, sizeof(json),
                "{\"role\":\"%s\",\"ntp\":%s,\"sd\":%s,\"webPort\":%u,"
                "\"hubStaMac\":\"%s\",\"bmsBridgeMac\":\"%s\",\"xtBridgeMac\":\"%s\","
                "\"ecoflowBridgeMac\":\"%s\"}",
                role, status.ntpSynced ? "true" : "false",
                historyStore_.isReady() ? "true" : "false",
                guest ? 0u : static_cast<unsigned>(status.webPort), hubMac, bmsMac, xtMac, ecoMac);
  sendJson(200, json);
}

}  // namespace wattcycle::web

