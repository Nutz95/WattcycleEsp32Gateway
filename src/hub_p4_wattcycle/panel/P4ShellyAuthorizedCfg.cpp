#include "P4ShellyAuthorizedCfg.h"

#include "Util/SafeCopy.h"

#include <SD_MMC.h>

#include <cstring>

namespace wattcycle::panel {
namespace {

bool appendLineEntry(shelly::ShellyDeviceIdentity* out, size_t capacity, size_t& count,
                     char* line) {
  if (count >= capacity || line == nullptr) {
    return false;
  }
  if (std::strcmp(line, "v1") == 0) {
    return false;
  }
  char* sep = std::strchr(line, '=');
  if (sep == nullptr || sep == line || sep[1] == '\0') {
    return false;
  }
  *sep = '\0';
  util::copyCString(out[count].id, sizeof(out[count].id), line);
  util::copyCString(out[count].name, sizeof(out[count].name), sep + 1);
  ++count;
  return true;
}

}  // namespace

size_t loadShellyAuthorizedCfg(const char* path, shelly::ShellyDeviceIdentity* out,
                               size_t capacity) {
  if (path == nullptr || out == nullptr || capacity == 0 || SD_MMC.cardType() == CARD_NONE) {
    return 0;
  }
  File file = SD_MMC.open(path, "r");
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
      appendLineEntry(out, capacity, count, line);
      continue;
    }
    if (used + 1 < sizeof(line)) {
      line[used++] = static_cast<char>(next);
    }
  }
  if (used > 0 && count < capacity) {
    line[used] = '\0';
    appendLineEntry(out, capacity, count, line);
  }
  file.close();
  return count;
}

bool saveShellyAuthorizedCfg(const char* path, const shelly::ShellyDeviceIdentity* entries,
                             size_t count) {
  if (path == nullptr || SD_MMC.cardType() == CARD_NONE) {
    return false;
  }
  File file = SD_MMC.open(path, "w");
  if (!file) {
    return false;
  }
  file.println("v1");
  for (size_t i = 0; i < count; ++i) {
    if (entries == nullptr || entries[i].id[0] == '\0') {
      continue;
    }
    file.print(entries[i].id);
    file.print('=');
    file.println(entries[i].name[0] != '\0' ? entries[i].name : entries[i].id);
  }
  file.close();
  return true;
}

}  // namespace wattcycle::panel
