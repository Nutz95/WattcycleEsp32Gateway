#include "Storage/HistoryCatalog.h"

#include <Arduino.h>

#include <cstdio>
#include <cstring>

namespace wattcycle::storage {
namespace {

const char* baseName(const char* name) {
  if (name == nullptr) {
    return "";
  }
  const char* slash = std::strrchr(name, '/');
  return slash == nullptr ? name : slash + 1;
}

bool digitsOf(const char* text, size_t length) {
  if (text == nullptr || std::strlen(text) != length) {
    return false;
  }
  for (size_t i = 0; i < length; ++i) {
    if (text[i] < '0' || text[i] > '9') {
      return false;
    }
  }
  return true;
}

bool isYmd(const char* text) {
  if (text == nullptr || std::strlen(text) != 10) {
    return false;
  }
  for (int i = 0; i < 10; ++i) {
    const bool dash = i == 4 || i == 7;
    if (dash && text[i] != '-') {
      return false;
    }
    if (!dash && (text[i] < '0' || text[i] > '9')) {
      return false;
    }
  }
  return true;
}

bool sameMonth(const char* ymd, int year, int month) {
  int fileYear = 0;
  int fileMonth = 0;
  int fileDay = 0;
  if (std::sscanf(ymd, "%d-%d-%d", &fileYear, &fileMonth, &fileDay) != 3) {
    return false;
  }
  return fileYear == year && fileMonth == month;
}

void sortDesc(char rows[][8], size_t count) {
  for (size_t i = 1; i < count; ++i) {
    char key[8] = {};
    std::memcpy(key, rows[i], sizeof(key));
    size_t slot = i;
    while (slot > 0 && std::strcmp(rows[slot - 1], key) < 0) {
      std::memcpy(rows[slot], rows[slot - 1], sizeof(key));
      --slot;
    }
    std::memcpy(rows[slot], key, sizeof(key));
  }
}

void sortDaysDesc(char rows[][11], size_t count) {
  for (size_t i = 1; i < count; ++i) {
    char key[11] = {};
    std::memcpy(key, rows[i], sizeof(key));
    size_t slot = i;
    while (slot > 0 && std::strcmp(rows[slot - 1], key) < 0) {
      std::memcpy(rows[slot], rows[slot - 1], sizeof(key));
      --slot;
    }
    std::memcpy(rows[slot], key, sizeof(key));
  }
}

bool pushPeriod(char out[][8], size_t& count, size_t maxPeriods, const char* key) {
  if (count >= maxPeriods) {
    return false;
  }
  for (size_t i = 0; i < count; ++i) {
    if (std::strcmp(out[i], key) == 0) {
      return true;
    }
  }
  std::snprintf(out[count], 8, "%s", key);
  ++count;
  return true;
}

bool pushDay(char out[][11], size_t& count, size_t maxDays, const char* ymd) {
  if (!isYmd(ymd) || count >= maxDays) {
    return false;
  }
  for (size_t i = 0; i < count; ++i) {
    if (std::strcmp(out[i], ymd) == 0) {
      return true;
    }
  }
  std::memcpy(out[count], ymd, 11);
  ++count;
  return true;
}

void addMonthDirs(fs::FS& fs, const char* yearName, char out[][8], size_t& count, size_t maxPeriods) {
  char yearPath[20] = {};
  std::snprintf(yearPath, sizeof(yearPath), "/history/%s", yearName);
  File months = fs.open(yearPath);
  if (!months) {
    return;
  }
  for (File month = months.openNextFile(); month; month = months.openNextFile()) {
    const bool directory = month.isDirectory();
    char monthName[4] = {};
    std::snprintf(monthName, sizeof(monthName), "%s", baseName(month.name()));
    month.close();
    if (!directory || !digitsOf(monthName, 2)) {
      continue;
    }
    char key[8] = {};
    std::snprintf(key, sizeof(key), "%s-%s", yearName, monthName);
    pushPeriod(out, count, maxPeriods, key);
  }
  months.close();
}

void addTreePeriods(fs::FS& fs, char out[][8], size_t& count, size_t maxPeriods) {
  File root = fs.open("/history");
  if (!root || !root.isDirectory()) {
    return;
  }
  for (File year = root.openNextFile(); year; year = root.openNextFile()) {
    const bool directory = year.isDirectory();
    char yearName[8] = {};
    std::snprintf(yearName, sizeof(yearName), "%s", baseName(year.name()));
    year.close();
    if (!directory || !digitsOf(yearName, 4)) {
      continue;
    }
    addMonthDirs(fs, yearName, out, count, maxPeriods);
  }
  root.close();
}

void addFlatPeriods(fs::FS& fs, char out[][8], size_t& count, size_t maxPeriods) {
  File root = fs.open("/history");
  if (!root) {
    return;
  }
  for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
    const bool directory = entry.isDirectory();
    char stem[16] = {};
    std::snprintf(stem, sizeof(stem), "%s", baseName(entry.name()));
    entry.close();
    if (directory || std::strlen(stem) < 14 || std::strcmp(stem + 10, ".csv") != 0) {
      continue;
    }
    stem[10] = '\0';
    if (!isYmd(stem)) {
      continue;
    }
    char key[8] = {};
    std::snprintf(key, sizeof(key), "%.4s-%.2s", stem, stem + 5);
    pushPeriod(out, count, maxPeriods, key);
  }
  root.close();
}

void addDaysIn(fs::FS& fs, const char* path, int year, int month, bool onlyMonth, char out[][11],
               size_t& count, size_t maxDays) {
  File dir = fs.open(path);
  if (!dir || !dir.isDirectory()) {
    return;
  }
  for (File entry = dir.openNextFile(); entry && count < maxDays; entry = dir.openNextFile()) {
    const bool directory = entry.isDirectory();
    char stem[16] = {};
    std::snprintf(stem, sizeof(stem), "%s", baseName(entry.name()));
    entry.close();
    if (directory || std::strlen(stem) < 14 || std::strcmp(stem + 10, ".csv") != 0) {
      continue;
    }
    stem[10] = '\0';
    if (!isYmd(stem) || (onlyMonth && !sameMonth(stem, year, month))) {
      continue;
    }
    pushDay(out, count, maxDays, stem);
  }
  dir.close();
}

}  // namespace

bool HistoryCatalog::ensureMonth(fs::FS& fs, int year, int month) {
  if (year < 2024 || month < 1 || month > 12) {
    return false;
  }
  if (!fs.exists("/history") && !fs.mkdir("/history")) {
    return false;
  }
  char yearPath[16] = {};
  std::snprintf(yearPath, sizeof(yearPath), "/history/%04d", year);
  if (!fs.exists(yearPath) && !fs.mkdir(yearPath)) {
    return false;
  }
  char monthPath[24] = {};
  std::snprintf(monthPath, sizeof(monthPath), "%s/%02d", yearPath, month);
  return fs.exists(monthPath) || fs.mkdir(monthPath);
}

void HistoryCatalog::formatDayPath(char* out, size_t capacity, const char* ymd) {
  if (out == nullptr || capacity == 0) {
    return;
  }
  if (ymd == nullptr) {
    out[0] = '\0';
    return;
  }
  std::snprintf(out, capacity, "/history/%.4s/%.2s/%s.csv", ymd, ymd + 5, ymd);
}

void HistoryCatalog::formatLegacyPath(char* out, size_t capacity, const char* ymd) {
  if (out == nullptr || capacity == 0) {
    return;
  }
  std::snprintf(out, capacity, "/history/%s.csv", ymd == nullptr ? "" : ymd);
}

bool HistoryCatalog::openDay(fs::FS& fs, const char* ymd, fs::File& file) {
  if (!isYmd(ymd)) {
    return false;
  }
  char path[40] = {};
  formatDayPath(path, sizeof(path), ymd);
  file = fs.open(path, FILE_READ);
  if (file) {
    return true;
  }
  formatLegacyPath(path, sizeof(path), ymd);
  file = fs.open(path, FILE_READ);
  return static_cast<bool>(file);
}

size_t HistoryCatalog::listPeriods(fs::FS& fs, char out[][8], size_t maxPeriods) {
  if (out == nullptr || maxPeriods == 0) {
    return 0;
  }
  size_t count = 0;
  addTreePeriods(fs, out, count, maxPeriods);
  addFlatPeriods(fs, out, count, maxPeriods);
  sortDesc(out, count);
  return count;
}

size_t HistoryCatalog::listMonthDays(fs::FS& fs, int year, int month, char out[][11], size_t maxDays) {
  if (out == nullptr || maxDays == 0 || year < 2024 || month < 1 || month > 12) {
    return 0;
  }
  char monthPath[24] = {};
  std::snprintf(monthPath, sizeof(monthPath), "/history/%04d/%02d", year, month);
  size_t count = 0;
  addDaysIn(fs, monthPath, year, month, false, out, count, maxDays);
  addDaysIn(fs, "/history", year, month, true, out, count, maxDays);
  sortDaysDesc(out, count);
  return count;
}

}  // namespace wattcycle::storage
