#pragma once

#include <cstdint>
#include <ctime>

namespace wattcycle::time_sync {

/// SNTP wall clock. Safe to construct before Wi-Fi; syncs once STA has an IP.
/// Polls are non-blocking (never stalls the hub loop). Once synced, stays synced
/// (no aggressive re-NTP that could disrupt web/history). Epoch files stay UTC.
class NtpClock {
 public:
  void begin(const char* posixTimeZone = "CET-1CEST,M3.5.0,M10.5.0/3");
  void loop(bool wifiConnected);
  bool isSynced() const;
  bool nowUtc(struct tm& out) const;
  std::time_t nowEpoch() const;
  /// "YYYY-MM-DD" (UTC), returns false if not synced.
  bool formatUtcDate(char* out, size_t capacity) const;
  /// Local wall clock for display (posix TZ from begin). date="DD/MM/YYYY", time="HH:MM".
  bool formatLocalDateTime(char* dateOut, size_t dateCapacity, char* timeOut,
                           size_t timeCapacity) const;

 private:
  bool begun_ = false;
  bool synced_ = false;
  uint32_t lastAttemptMs_ = 0;
  char posixTimeZone_[48] = "CET-1CEST,M3.5.0,M10.5.0/3";
};

}  // namespace wattcycle::time_sync
