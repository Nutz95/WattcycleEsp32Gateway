#include "P4ShellyConfigStore.h"

#include "Util/SafeCopy.h"

#include <SD_MMC.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace wattcycle::panel {

size_t P4ShellyConfigStore::loadAuthorized(shelly::ShellyPlugIdentity* out, size_t capacity) {
  if (out == nullptr || capacity == 0 || SD_MMC.cardType() == CARD_NONE) {
    return 0;
  }
  File file = SD_MMC.open("/plugs.cfg", "r");
  if (!file) {
    return 0;
  }
  size_t count = 0;
  char line[96] = {};
  size_t used = 0;
  while (file.available() && count < capacity) {
    const int next = file.read();
    if (next < 0) {
      break;
    }
    if (next == '\n' || next == '\r') {
      if (used == 0) {
        continue;
      }
      line[used] = '\0';
      used = 0;
      if (std::strcmp(line, "v1") == 0) {
        continue;
      }
      char* sep = std::strchr(line, '=');
      if (sep == nullptr || sep == line || sep[1] == '\0') {
        continue;
      }
      *sep = '\0';
      util::copyCString(out[count].id, sizeof(out[count].id), line);
      util::copyCString(out[count].name, sizeof(out[count].name), sep + 1);
      ++count;
      continue;
    }
    if (used + 1 < sizeof(line)) {
      line[used++] = static_cast<char>(next);
    }
  }
  if (used > 0 && count < capacity) {
    line[used] = '\0';
    char* sep = std::strchr(line, '=');
    if (sep != nullptr && sep != line && sep[1] != '\0') {
      *sep = '\0';
      util::copyCString(out[count].id, sizeof(out[count].id), line);
      util::copyCString(out[count].name, sizeof(out[count].name), sep + 1);
      ++count;
    }
  }
  file.close();
  return count;
}

bool P4ShellyConfigStore::saveAuthorized(const shelly::ShellyPlugIdentity* entries, size_t count) {
  if (SD_MMC.cardType() == CARD_NONE) {
    return false;
  }
  File file = SD_MMC.open("/plugs.cfg", "w");
  if (!file) {
    return false;
  }
  file.println("v1");
  for (size_t i = 0; i < count; ++i) {
    if (entries[i].id[0] == '\0') {
      continue;
    }
    file.print(entries[i].id);
    file.print('=');
    file.println(entries[i].name[0] != '\0' ? entries[i].name : entries[i].id);
  }
  file.close();
  return true;
}

bool P4ShellyConfigStore::loadDayBaseline(uint32_t& dayYyyymmdd, float* whBySlot,
                                          char ids[][shelly::kShellyIdCapacity], size_t capacity,
                                          size_t& count) {
  dayYyyymmdd = 0;
  count = 0;
  if (whBySlot == nullptr || ids == nullptr || capacity == 0 || SD_MMC.cardType() == CARD_NONE) {
    return false;
  }
  File file = SD_MMC.open("/plugs.day", "r");
  if (!file) {
    return false;
  }
  char line[96] = {};
  size_t used = 0;
  bool haveDay = false;
  while (file.available() && count < capacity) {
    const int next = file.read();
    if (next < 0) {
      break;
    }
    if (next == '\n' || next == '\r') {
      if (used == 0) {
        continue;
      }
      line[used] = '\0';
      used = 0;
      if (!haveDay) {
        unsigned year = 0;
        unsigned month = 0;
        unsigned day = 0;
        if (std::sscanf(line, "%u-%u-%u", &year, &month, &day) == 3) {
          dayYyyymmdd = year * 10000u + month * 100u + day;
          haveDay = true;
        }
        continue;
      }
      char* sep = std::strchr(line, '=');
      if (sep == nullptr || sep == line) {
        continue;
      }
      *sep = '\0';
      util::copyCString(ids[count], shelly::kShellyIdCapacity, line);
      whBySlot[count] = static_cast<float>(std::strtod(sep + 1, nullptr));
      ++count;
      continue;
    }
    if (used + 1 < sizeof(line)) {
      line[used++] = static_cast<char>(next);
    }
  }
  file.close();
  return haveDay;
}

bool P4ShellyConfigStore::saveDayBaseline(uint32_t dayYyyymmdd, const float* whBySlot,
                                          const char ids[][shelly::kShellyIdCapacity],
                                          size_t count) {
  if (whBySlot == nullptr || ids == nullptr || SD_MMC.cardType() == CARD_NONE ||
      dayYyyymmdd == 0) {
    return false;
  }
  File file = SD_MMC.open("/plugs.day", "w");
  if (!file) {
    return false;
  }
  char header[16] = {};
  std::snprintf(header, sizeof(header), "%04u-%02u-%02u",
                static_cast<unsigned>(dayYyyymmdd / 10000u),
                static_cast<unsigned>((dayYyyymmdd / 100u) % 100u),
                static_cast<unsigned>(dayYyyymmdd % 100u));
  file.println(header);
  for (size_t i = 0; i < count; ++i) {
    if (ids[i][0] == '\0') {
      continue;
    }
    file.print(ids[i]);
    file.print('=');
    file.println(whBySlot[i], 3);
  }
  file.close();
  return true;
}

}  // namespace wattcycle::panel
