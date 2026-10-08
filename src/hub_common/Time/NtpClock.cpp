#include "Time/NtpClock.h"

#include "Util/SafeCopy.h"

#include <cstdio>
#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <time.h>
#endif

namespace wattcycle::time_sync {
namespace {
#ifndef UNIT_TEST
constexpr uint32_t kRetryMs = 60000;
constexpr const char* kNtpServer = "pool.ntp.org";

void applyTimeZone(const char* posixTz) {
  if (posixTz == nullptr || posixTz[0] == '\0') {
    setenv("TZ", "UTC0", 1);
  } else {
    setenv("TZ", posixTz, 1);
  }
  tzset();
}
#endif
}  // namespace

void NtpClock::begin(const char* posixTimeZone) {
#ifndef UNIT_TEST
  wattcycle::util::copyCString(posixTimeZone_, sizeof(posixTimeZone_),
                               posixTimeZone != nullptr ? posixTimeZone : "UTC0");
  // Keep libc epoch UTC; local formatting applies posixTimeZone_ via TZ.
  configTime(0, 0, kNtpServer);
  applyTimeZone(posixTimeZone_);
  begun_ = true;
#else
  (void)posixTimeZone;
  begun_ = true;
#endif
}

void NtpClock::loop(bool wifiConnected) {
#ifndef UNIT_TEST
  if (!begun_ || !wifiConnected || synced_) {
    return;
  }
  const uint32_t now = millis();
  if (lastAttemptMs_ != 0 && (now - lastAttemptMs_) < kRetryMs) {
    return;
  }
  lastAttemptMs_ = now;
  struct tm info = {};
  if (getLocalTime(&info, 2000)) {
    synced_ = true;
    Serial.printf("NTP OK local %04d-%02d-%02d %02d:%02d TZ=%s\n", info.tm_year + 1900,
                  info.tm_mon + 1, info.tm_mday, info.tm_hour, info.tm_min, posixTimeZone_);
  } else {
    Serial.println(F("NTP sync pending"));
  }
#else
  (void)wifiConnected;
#endif
}

bool NtpClock::isSynced() const {
  return synced_;
}

bool NtpClock::nowUtc(struct tm& out) const {
#ifndef UNIT_TEST
  if (!synced_) {
    return false;
  }
  const std::time_t epoch = std::time(nullptr);
  return gmtime_r(&epoch, &out) != nullptr;
#else
  (void)out;
  return false;
#endif
}

std::time_t NtpClock::nowEpoch() const {
#ifndef UNIT_TEST
  if (!synced_) {
    return 0;
  }
  return std::time(nullptr);
#else
  return 0;
#endif
}

bool NtpClock::formatUtcDate(char* out, size_t capacity) const {
  if (out == nullptr || capacity < 11) {
    return false;
  }
  struct tm info = {};
  if (!nowUtc(info)) {
    out[0] = '\0';
    return false;
  }
  std::snprintf(out, capacity, "%04d-%02d-%02d", info.tm_year + 1900, info.tm_mon + 1,
                info.tm_mday);
  return true;
}

bool NtpClock::formatLocalDateTime(char* dateOut, size_t dateCapacity, char* timeOut,
                                   size_t timeCapacity) const {
#ifndef UNIT_TEST
  if (!synced_ || dateOut == nullptr || timeOut == nullptr || dateCapacity < 11 ||
      timeCapacity < 6) {
    return false;
  }
  applyTimeZone(posixTimeZone_);
  struct tm info = {};
  if (!getLocalTime(&info, 0)) {
    return false;
  }
  std::snprintf(dateOut, dateCapacity, "%02d/%02d/%04d", info.tm_mday, info.tm_mon + 1,
                info.tm_year + 1900);
  std::snprintf(timeOut, timeCapacity, "%02d:%02d", info.tm_hour, info.tm_min);
  return true;
#else
  (void)dateOut;
  (void)dateCapacity;
  (void)timeOut;
  (void)timeCapacity;
  return false;
#endif
}

}  // namespace wattcycle::time_sync
