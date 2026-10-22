#include "HttpApi/EspWebGateway.h"

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

void choosePeriod(const char periods[][8], size_t count, int& year, int& month) {
  int nowYear = 0;
  int nowMonth = 0;
  currentUtc(nowYear, nowMonth);
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

size_t appendYears(char* json, size_t cap, size_t pos, const char periods[][8], size_t count,
                   int nowYear) {
  pos = appendText(json, cap, pos, "\"years\":[");
  bool first = true;
  int previous = 0;
  if (nowYear >= 2024) {
    pos += static_cast<size_t>(std::snprintf(json + pos, cap - pos, "%d", nowYear));
    first = false;
    previous = nowYear;
  }
  for (size_t i = 0; i < count && pos + 8 < cap; ++i) {
    const int year = periodYear(periods[i]);
    if (year < 2024 || year == previous) {
      continue;
    }
    pos += static_cast<size_t>(
        std::snprintf(json + pos, cap - pos, "%s%d", first ? "" : ",", year));
    first = false;
    previous = year;
  }
  return appendText(json, cap, pos, "]");
}

size_t appendMonths(char* json, size_t cap, size_t pos, const char periods[][8], size_t count,
                    int year, int extraMonth) {
  int months[12] = {};
  size_t monthCount = 0;
  if (extraMonth >= 1 && extraMonth <= 12) {
    months[monthCount++] = extraMonth;
  }
  for (size_t i = 0; i < count && monthCount < 12; ++i) {
    if (periodYear(periods[i]) != year) {
      continue;
    }
    const int month = periodMonth(periods[i]);
    bool seen = false;
    for (size_t j = 0; j < monthCount; ++j) {
      seen = seen || months[j] == month;
    }
    if (!seen && month >= 1 && month <= 12) {
      months[monthCount++] = month;
    }
  }
  for (size_t i = 1; i < monthCount; ++i) {
    const int key = months[i];
    size_t slot = i;
    while (slot > 0 && months[slot - 1] < key) {
      months[slot] = months[slot - 1];
      --slot;
    }
    months[slot] = key;
  }
  pos = appendText(json, cap, pos, "\"months\":[");
  for (size_t i = 0; i < monthCount && pos + 6 < cap; ++i) {
    pos += static_cast<size_t>(
        std::snprintf(json + pos, cap - pos, "%s%d", i == 0 ? "" : ",", months[i]));
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
  if (!requireAuth()) {
    return;
  }
  static char periods[48][8];
  static char days[31][11];
  static char json[960];
  const bool sd = historyStore_.isReady();
  const size_t periodCount = sd ? historyStore_.listPeriods(periods, 48) : 0;
  int year = server_->hasArg("year") ? server_->arg("year").toInt() : 0;
  int month = server_->hasArg("month") ? server_->arg("month").toInt() : 0;
  choosePeriod(periods, periodCount, year, month);
  const size_t dayCount =
      sd && year >= 2024 ? historyStore_.listMonthDays(year, month, days, 31) : 0;
  int nowYear = 0;
  int nowMonth = 0;
  currentUtc(nowYear, nowMonth);
  const bool ntp = store_.status().ntpSynced;
  size_t pos = static_cast<size_t>(std::snprintf(
      json, sizeof(json), "{\"ntp\":%s,\"sd\":%s,\"year\":%d,\"month\":%d,",
      ntp ? "true" : "false", sd ? "true" : "false", year, month));
  pos = appendYears(json, sizeof(json), pos, periods, periodCount, nowYear);
  pos = appendText(json, sizeof(json), pos, ",");
  pos = appendMonths(json, sizeof(json), pos, periods, periodCount, year,
                     year == nowYear ? nowMonth : 0);
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
  historyStore_.streamDay(date.c_str(), &beginDayStream, &writeDayChunk, &ctx);
  if (!ctx.headerSent) {
    sendJson(404, "{\"error\":\"day_not_found\"}");
  }
}

void EspWebGateway::handleDeviceInfo() {
  if (!requireAuth()) {
    return;
  }
  const char* role = role_[0] != '\0' ? role_ : "hub";
  const auto status = store_.status();
  char json[512] = {};
  std::snprintf(json, sizeof(json),
                "{\"role\":\"%s\",\"ntp\":%s,\"sd\":%s,\"c6HeapKb\":%d,"
                "\"hubStaMac\":\"%s\",\"bmsBridgeMac\":\"%s\",\"xtBridgeMac\":\"%s\","
                "\"ecoflowBridgeMac\":\"%s\"}",
                role, status.ntpSynced ? "true" : "false",
                historyStore_.isReady() ? "true" : "false", status.coprocessorHeapKb, hubStaMac_,
                bmsBridgeMac_, xtBridgeMac_, ecoflowBridgeMac_);
  sendJson(200, json);
}

}  // namespace wattcycle::web

