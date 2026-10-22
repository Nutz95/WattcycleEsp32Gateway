#pragma once

#include <FS.h>

#include <cstddef>

namespace wattcycle::storage {

/// FAT day files at /history/YYYY/MM/YYYY-MM-DD.csv. Flat /history/YYYY-MM-DD.csv stays readable.
class HistoryCatalog {
 public:
  /// Create /history/YYYY/MM. False when a mkdir fails.
  static bool ensureMonth(fs::FS& fs, int year, int month);
  /// Nested path for one UTC day. `ymd` is "YYYY-MM-DD".
  static void formatDayPath(char* out, size_t capacity, const char* ymd);
  /// Legacy flat path kept for cards written before the year/month folders.
  static void formatLegacyPath(char* out, size_t capacity, const char* ymd);
  /// Open the nested file, then the legacy file. False when neither exists.
  static bool openDay(fs::FS& fs, const char* ymd, fs::File& file);
  /// Fills out[i] with "YYYY-MM", newest first.
  static size_t listPeriods(fs::FS& fs, char out[][8], size_t maxPeriods);
  /// Fills out[i] with "YYYY-MM-DD" for one month, newest first.
  static size_t listMonthDays(fs::FS& fs, int year, int month, char out[][11], size_t maxDays);
};

}  // namespace wattcycle::storage
