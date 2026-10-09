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
/// Retry interval while waiting for first SNTP answer (non-blocking polls).
constexpr uint32_t kRetryMs = 15000;
constexpr const char* kNtpServer = "pool.ntp.org";
/// Epoch sanity: refuse "synced" until wall clock is past 2024-01-01 UTC.
constexpr std::time_t kMinPlausibleEpoch = 1704067200;

void applyTimeZone(const char* posixTz) {
  if (posixTz == nullptr || posixTz[0] == '\0') {
    setenv("TZ", "UTC0", 1);
  } else {
    setenv("TZ", posixTz, 1);
  }
  tzset();
}

bool wallClockLooksValid() {
  const std::time_t epoch = std::time(nullptr);
  return epoch >= kMinPlausibleEpoch;
}
#endif
}  // namespace

void NtpClock::begin(const char* posixTimeZone) {
#ifndef UNIT_TEST
  wattcycle::util::copyCString(posixTimeZone_, sizeof(posixTimeZone_),
                               posixTimeZone != nullptr ? posixTimeZone : "UTC0");
  // Keep libc epoch UTC; local formatting applies posixTimeZone_ via TZ.
  // Do not block here — SNTP runs in the background after STA is up.
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

  // Non-blocking: timeout 0 — never stall the M5 main loop / web / ESP-NOW.
  struct tm info = {};
  if (getLocalTime(&info, 0) && wallClockLooksValid()) {
    synced_ = true;
    Serial.printf("NTP OK local %04d-%02d-%02d %02d:%02d TZ=%s\n", info.tm_year + 1900,
                  info.tm_mon + 1, info.tm_mday, info.tm_hour, info.tm_min, posixTimeZone_);
  } else {
    Serial.println(F("NTP sync pending (non-blocking)"));
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
  if (!synced_ || !wallClockLooksValid()) {
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
  if (!synced_ || !wallClockLooksValid()) {
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
      timeCapacity < 6 || !wallClockLooksValid()) {
    return false;
  }
  // TZ already applied in begin(); do not setenv/tzset on every UI tick.
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
