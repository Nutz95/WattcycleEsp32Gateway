#include "Shelly/ShellySmokeHub.h"

#include "Shelly/ShellyDisplayName.h"
#include "Shelly/ShellySmokeId.h"
#include "Util/SafeCopy.h"

#include <cstring>

namespace wattcycle::shelly {

int ShellySmokeHub::findById(const char* id) const {
  if (id == nullptr || id[0] == '\0') {
    return -1;
  }
  for (size_t i = 0; i < kMaxShellySmoke; ++i) {
    if (slots_[i].used && std::strcmp(slots_[i].identity.id, id) == 0) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int ShellySmokeHub::findFreeSlot() const {
  for (size_t i = 0; i < kMaxShellySmoke; ++i) {
    if (!slots_[i].used) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool ShellySmokeHub::persistAuthorized() {
  if (store_ == nullptr) {
    return true;
  }
  ShellyDeviceIdentity entries[kMaxShellySmoke] = {};
  size_t count = 0;
  for (size_t i = 0; i < kMaxShellySmoke; ++i) {
    if (!slots_[i].used || !slots_[i].authorized) {
      continue;
    }
    entries[count++] = slots_[i].identity;
  }
  return store_->saveAuthorized(entries, count);
}

bool ShellySmokeHub::authorize(const char* id, const char* name) {
  if (!shellySmokeIdValid(id)) {
    return false;
  }
  char label[kShellyNameCapacity] = {};
  if (!ShellyDisplayName::sanitize(name, label, sizeof(label))) {
    ShellyDisplayName::shortFromId(id, label, sizeof(label));
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
  ShellySmokeSlot& slot = slots_[static_cast<size_t>(index)];
  size_t authorized = 0;
  for (size_t i = 0; i < kMaxShellySmoke; ++i) {
    if (slots_[i].used && slots_[i].authorized) {
      ++authorized;
    }
  }
  if (!slot.authorized && authorized >= kMaxShellySmoke) {
    return false;
  }
  slot.authorized = true;
  util::copyCString(slot.identity.name, sizeof(slot.identity.name), label);
  util::copyCString(slot.reading.name, sizeof(slot.reading.name), label);
  slot.reading.authorized = true;
  markDirty();
  return persistAuthorized();
}

bool ShellySmokeHub::rename(const char* id, const char* name) {
  const int index = findById(id);
  if (index < 0 || !slots_[static_cast<size_t>(index)].authorized) {
    return false;
  }
  char label[kShellyNameCapacity] = {};
  if (!ShellyDisplayName::sanitize(name, label, sizeof(label))) {
    return false;
  }
  ShellySmokeSlot& slot = slots_[static_cast<size_t>(index)];
  util::copyCString(slot.identity.name, sizeof(slot.identity.name), label);
  util::copyCString(slot.reading.name, sizeof(slot.reading.name), label);
  markDirty();
  return persistAuthorized();
}

bool ShellySmokeHub::revoke(const char* id) {
  const int index = findById(id);
  if (index < 0) {
    return false;
  }
  ShellySmokeSlot& slot = slots_[static_cast<size_t>(index)];
  slot.authorized = false;
  slot.reading.authorized = false;
  if (!slot.reading.hasAlarm && slot.reading.lastMs == 0) {
    slot.used = false;
    slot = {};
  }
  markDirty();
  return persistAuthorized();
}

bool ShellySmokeHub::copyName(const char* id, char* out, size_t capacity) const {
  if (out == nullptr || capacity == 0) {
    return false;
  }
  out[0] = '\0';
  const int index = findById(id);
  if (index < 0 || !slots_[static_cast<size_t>(index)].authorized) {
    return false;
  }
  const ShellySmokeSlot& slot = slots_[static_cast<size_t>(index)];
  if (slot.identity.name[0] != '\0') {
    util::copyCString(out, capacity, slot.identity.name);
  } else {
    ShellyDisplayName::shortFromId(slot.identity.id, out, capacity);
  }
  return out[0] != '\0';
}

void ShellySmokeHub::fillReading(const ShellySmokeSlot& slot, ShellySmokeReading& out,
                                 uint32_t nowMs) const {
  out = slot.reading;
  out.authorized = slot.authorized;
  util::copyCString(out.id, sizeof(out.id), slot.identity.id);
  util::copyCString(out.name, sizeof(out.name), slot.identity.name);
  if (out.name[0] == '\0') {
    ShellyDisplayName::shortFromId(slot.identity.id, out.name, sizeof(out.name));
  }
  out.online = slot.reading.lastMs != 0 &&
               static_cast<uint32_t>(nowMs - slot.reading.lastMs) <= kShellySmokeStaleMs;
}

size_t ShellySmokeHub::snapshotAuthorized(ShellySmokeReading* out, size_t capacity,
                                          uint32_t nowMs) const {
  if (out == nullptr || capacity == 0) {
    return 0;
  }
  size_t written = 0;
  for (size_t i = 0; i < kMaxShellySmoke && written < capacity; ++i) {
    if (!slots_[i].used || !slots_[i].authorized) {
      continue;
    }
    fillReading(slots_[i], out[written], nowMs);
    ++written;
  }
  return written;
}

size_t ShellySmokeHub::snapshotPosting(ShellySmokeReading* out, size_t capacity,
                                       uint32_t nowMs) const {
  if (out == nullptr || capacity == 0) {
    return 0;
  }
  size_t written = 0;
  for (size_t i = 0; i < kMaxShellySmoke && written < capacity; ++i) {
    if (!slots_[i].used || slots_[i].authorized || slots_[i].reading.lastMs == 0) {
      continue;
    }
    if (static_cast<uint32_t>(nowMs - slots_[i].reading.lastMs) > kShellySmokePostingTtlMs) {
      continue;
    }
    fillReading(slots_[i], out[written], nowMs);
    ++written;
  }
  return written;
}

bool ShellySmokeHub::consumeDirty() {
  const bool was = dirty_;
  dirty_ = false;
  return was;
}

void ShellySmokeHub::markDirty() {
  dirty_ = true;
}

}  // namespace wattcycle::shelly
