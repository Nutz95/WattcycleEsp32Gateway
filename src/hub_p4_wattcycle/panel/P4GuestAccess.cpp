#include "P4GuestAccess.h"

#include <SD_MMC.h>

#include <cstdio>
#include <cstdlib>

namespace wattcycle::panel {

void P4GuestAccess::begin() {
  enabled_ = false;
  tabs_ = kDefaultTabs;
  if (SD_MMC.cardType() == CARD_NONE) {
    return;
  }
  File file = SD_MMC.open("/guest.cfg", "r");
  if (!file) {
    return;
  }
  const int stored = file.read();
  enabled_ = stored == '1';
  char line[16] = {};
  size_t used = 0;
  while (file.available() && used + 1 < sizeof(line)) {
    const int next = file.read();
    if (next == '\n' && used == 0) {
      continue;
    }
    if (next == '\n' || next == '\r') {
      break;
    }
    line[used++] = static_cast<char>(next);
  }
  file.close();
  if (used > 0) {
    tabs_ = static_cast<uint32_t>(std::strtoul(line, nullptr, 16)) & kTabMask;
  }
}

bool P4GuestAccess::enabled() const {
  return enabled_;
}

uint32_t P4GuestAccess::tabs() const {
  return tabs_;
}

bool P4GuestAccess::store() {
  if (SD_MMC.cardType() == CARD_NONE) {
    return false;
  }
  File file = SD_MMC.open("/guest.cfg", "w");
  if (!file) {
    return false;
  }
  char body[24] = {};
  std::snprintf(body, sizeof(body), "%c\n%lx\n", enabled_ ? '1' : '0', static_cast<unsigned long>(tabs_));
  const bool wrote = file.print(body) > 0;
  file.close();
  return wrote;
}

bool P4GuestAccess::setEnabled(bool on) {
  return update(on, tabs_);
}

bool P4GuestAccess::setTabs(uint32_t tabs) {
  return update(enabled_, tabs);
}

bool P4GuestAccess::update(bool on, uint32_t tabs) {
  const bool previousOn = enabled_;
  const uint32_t previousTabs = tabs_;
  enabled_ = on;
  tabs_ = tabs & kTabMask;
  if (!store()) {
    enabled_ = previousOn;
    tabs_ = previousTabs;
    return false;
  }
  return true;
}

}  // namespace wattcycle::panel
