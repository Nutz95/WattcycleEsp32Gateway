#include "Shelly/ShellyPlugHub.h"

#include "Shelly/ShellyPlugId.h"
#include "Util/SafeCopy.h"

#include <cstring>

namespace wattcycle::shelly {

bool ShellyPlugHub::sanitizeName(const char* name, char* out, size_t capacity) const {
  if (out == nullptr || capacity == 0) {
    return false;
  }
  out[0] = '\0';
  if (name == nullptr) {
    return false;
  }
  size_t written = 0;
  for (const char* cursor = name; *cursor != '\0' && written + 1 < capacity; ++cursor) {
    const unsigned char ch = static_cast<unsigned char>(*cursor);
    if (ch < 32u || ch == 127u || ch == '|' || ch == '\n' || ch == '\r' || ch == '"' ||
        ch == '\\' || ch == ',') {
      continue;
    }
    out[written++] = static_cast<char>(ch);
  }
  out[written] = '\0';
  return written > 0;
}

bool ShellyPlugHub::shortNameFromId(const char* id, char* out, size_t capacity) const {
  if (id == nullptr) {
    return false;
  }
  const char* dash = std::strrchr(id, '-');
  const char* tail = dash != nullptr && dash[1] != '\0' ? dash + 1 : id;
  util::copyCString(out, capacity, tail);
  return out[0] != '\0';
}

int ShellyPlugHub::findById(const char* id) const {
  if (id == nullptr || id[0] == '\0') {
    return -1;
  }
  for (size_t i = 0; i < kMaxShellyPlugs; ++i) {
    if (slots_[i].used && std::strcmp(slots_[i].identity.id, id) == 0) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int ShellyPlugHub::findFreeSlot() const {
  for (size_t i = 0; i < kMaxShellyPlugs; ++i) {
    if (!slots_[i].used) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool ShellyPlugHub::persistAuthorized() {
  if (store_ == nullptr) {
    return true;
  }
  ShellyPlugIdentity entries[kMaxShellyPlugs] = {};
  size_t count = 0;
  for (size_t i = 0; i < kMaxShellyPlugs; ++i) {
    if (!slots_[i].used || !slots_[i].authorized) {
      continue;
    }
    entries[count++] = slots_[i].identity;
  }
  return store_->saveAuthorized(entries, count);
}

bool ShellyPlugHub::persistDay() {
  if (store_ == nullptr || dayYyyymmdd_ == 0) {
    return true;
  }
  float baselines[kMaxShellyPlugs] = {};
  char ids[kMaxShellyPlugs][kShellyIdCapacity] = {};
  size_t count = 0;
  for (size_t i = 0; i < kMaxShellyPlugs; ++i) {
    if (!slots_[i].used || !slots_[i].authorized || !slots_[i].hasBaseline) {
      continue;
    }
    util::copyCString(ids[count], sizeof(ids[count]), slots_[i].identity.id);
    baselines[count] = slots_[i].dayBaselineWh;
    ++count;
  }
  return store_->saveDayBaseline(dayYyyymmdd_, baselines, ids, count);
}

bool ShellyPlugHub::authorize(const char* id, const char* name) {
  if (!shellyPlugIdValid(id)) {
    return false;
  }
  char label[kShellyNameCapacity] = {};
  if (!sanitizeName(name, label, sizeof(label))) {
    shortNameFromId(id, label, sizeof(label));
  }
  int index = findById(id);
  if (index < 0) {
    index = findFreeSlot();
    if (index < 0) {
      return false;
    }
    slots_[static_cast<size_t>(index)] = {};
    slots_[static_cast<size_t>(index)].used = true;
    util::copyCString(slots_[static_cast<size_t>(index)].identity.id,
                      sizeof(slots_[static_cast<size_t>(index)].identity.id), id);
    util::copyCString(slots_[static_cast<size_t>(index)].reading.id,
                      sizeof(slots_[static_cast<size_t>(index)].reading.id), id);
  }
  ShellyPlugSlot& slot = slots_[static_cast<size_t>(index)];
  size_t authorized = 0;
  for (size_t i = 0; i < kMaxShellyPlugs; ++i) {
    if (slots_[i].used && slots_[i].authorized) {
      ++authorized;
    }
  }
  if (!slot.authorized && authorized >= kMaxShellyPlugs) {
    return false;
  }
  slot.authorized = true;
  util::copyCString(slot.identity.name, sizeof(slot.identity.name), label);
  util::copyCString(slot.reading.name, sizeof(slot.reading.name), label);
  slot.reading.authorized = true;
  markDirty();
  return persistAuthorized();
}

bool ShellyPlugHub::rename(const char* id, const char* name) {
  const int index = findById(id);
  if (index < 0 || !slots_[static_cast<size_t>(index)].authorized) {
    return false;
  }
  char label[kShellyNameCapacity] = {};
  if (!sanitizeName(name, label, sizeof(label))) {
    return false;
  }
  ShellyPlugSlot& slot = slots_[static_cast<size_t>(index)];
  util::copyCString(slot.identity.name, sizeof(slot.identity.name), label);
  util::copyCString(slot.reading.name, sizeof(slot.reading.name), label);
  markDirty();
  return persistAuthorized();
}

bool ShellyPlugHub::revoke(const char* id) {
  const int index = findById(id);
  if (index < 0) {
    return false;
  }
  ShellyPlugSlot& slot = slots_[static_cast<size_t>(index)];
  slot.authorized = false;
  slot.reading.authorized = false;
  slot.hasBaseline = false;
  if (!slot.reading.hasWatts && slot.reading.lastMs == 0) {
    slot.used = false;
    slot = {};
  }
  markDirty();
  return persistAuthorized() && persistDay();
}

bool ShellyPlugHub::copyName(const char* id, char* out, size_t capacity) const {
  if (out == nullptr || capacity == 0) {
    return false;
  }
  out[0] = '\0';
  const int index = findById(id);
  if (index < 0 || !slots_[static_cast<size_t>(index)].authorized) {
    return false;
  }
  const ShellyPlugSlot& slot = slots_[static_cast<size_t>(index)];
  if (slot.identity.name[0] != '\0') {
    util::copyCString(out, capacity, slot.identity.name);
  } else {
    shortNameFromId(slot.identity.id, out, capacity);
  }
  return out[0] != '\0';
}

void ShellyPlugHub::fillReading(const ShellyPlugSlot& slot, ShellyPlugReading& out,
                                uint32_t nowMs) const {
  out = slot.reading;
  out.authorized = slot.authorized;
  util::copyCString(out.id, sizeof(out.id), slot.identity.id);
  util::copyCString(out.name, sizeof(out.name), slot.identity.name);
  if (out.name[0] == '\0') {
    shortNameFromId(slot.identity.id, out.name, sizeof(out.name));
  }
  out.online = slot.reading.lastMs != 0 &&
               static_cast<uint32_t>(nowMs - slot.reading.lastMs) <= kShellyOfflineMs;
}

size_t ShellyPlugHub::snapshotAuthorized(ShellyPlugReading* out, size_t capacity,
                                         uint32_t nowMs) const {
  if (out == nullptr || capacity == 0) {
    return 0;
  }
  size_t written = 0;
  for (size_t i = 0; i < kMaxShellyPlugs && written < capacity; ++i) {
    if (!slots_[i].used || !slots_[i].authorized) {
      continue;
    }
    fillReading(slots_[i], out[written], nowMs);
    ++written;
  }
  return written;
}

size_t ShellyPlugHub::snapshotPosting(ShellyPlugReading* out, size_t capacity,
                                      uint32_t nowMs) const {
  if (out == nullptr || capacity == 0) {
    return 0;
  }
  size_t written = 0;
  for (size_t i = 0; i < kMaxShellyPlugs && written < capacity; ++i) {
    if (!slots_[i].used || slots_[i].authorized || slots_[i].reading.lastMs == 0) {
      continue;
    }
    if (static_cast<uint32_t>(nowMs - slots_[i].reading.lastMs) > kShellyPostingTtlMs) {
      continue;
    }
    fillReading(slots_[i], out[written], nowMs);
    ++written;
  }
  return written;
}

bool ShellyPlugHub::consumeDirty() {
  const bool was = dirty_;
  dirty_ = false;
  return was;
}

void ShellyPlugHub::markDirty() {
  dirty_ = true;
}

}  // namespace wattcycle::shelly
