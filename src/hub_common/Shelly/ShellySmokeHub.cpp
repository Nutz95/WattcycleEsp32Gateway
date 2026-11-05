#include "Shelly/ShellySmokeHub.h"

#include "Shelly/ShellyDeviceIdentity.h"
#include "Shelly/ShellyDisplayName.h"
#include "Shelly/ShellySmokeId.h"
#include "Shelly/ShellySmokeNotifyParser.h"
#include "Util/SafeCopy.h"

#include <cstring>

namespace wattcycle::shelly {

ShellySmokeHub::ShellySmokeHub(IShellySmokeConfigStore* store) : store_(store) {}

void ShellySmokeHub::begin() {
  for (size_t i = 0; i < kMaxShellySmoke; ++i) {
    slots_[i] = {};
  }
  if (store_ == nullptr) {
    return;
  }
  ShellyDeviceIdentity loaded[kMaxShellySmoke] = {};
  const size_t count = store_->loadAuthorized(loaded, kMaxShellySmoke);
  size_t slot = 0;
  for (size_t i = 0; i < count && slot < kMaxShellySmoke; ++i) {
    if (!shellySmokeIdValid(loaded[i].id)) {
      continue;
    }
    slots_[slot].used = true;
    slots_[slot].authorized = true;
    slots_[slot].identity = loaded[i];
    util::copyCString(slots_[slot].reading.id, sizeof(slots_[slot].reading.id), loaded[i].id);
    util::copyCString(slots_[slot].reading.name, sizeof(slots_[slot].reading.name),
                      loaded[i].name);
    slots_[slot].reading.authorized = true;
    ++slot;
  }
}

bool ShellySmokeHub::listenOpen() const {
  return listenOpen_;
}

void ShellySmokeHub::setListen(bool open, uint32_t nowMs) {
  listenOpen_ = open;
  listenUntilMs_ = open ? nowMs + kShellyListenMs : 0;
  markDirty();
}

void ShellySmokeHub::expireListen(uint32_t nowMs) {
  if (listenOpen_ && listenUntilMs_ != 0 && static_cast<int32_t>(nowMs - listenUntilMs_) >= 0) {
    listenOpen_ = false;
    listenUntilMs_ = 0;
    markDirty();
  }
}

void ShellySmokeHub::applyDelta(ShellySmokeSlot& slot, const ShellySmokeNotifyDelta& delta,
                                uint32_t nowMs) {
  util::copyCString(slot.identity.id, sizeof(slot.identity.id), delta.id);
  util::copyCString(slot.reading.id, sizeof(slot.reading.id), delta.id);
  if (delta.hasAlarm || delta.full) {
    slot.reading.alarm = delta.hasAlarm ? delta.alarm : false;
    slot.reading.hasAlarm = delta.hasAlarm || delta.full;
  }
  if (delta.hasMute || delta.full) {
    slot.reading.mute = delta.hasMute ? delta.mute : false;
    slot.reading.hasMute = delta.hasMute || delta.full;
  }
  if (delta.hasBatteryPercent || delta.full) {
    slot.reading.batteryPercent = delta.hasBatteryPercent ? delta.batteryPercent : 0;
    slot.reading.hasBatteryPercent = delta.hasBatteryPercent || delta.full;
  }
  if (delta.hasBatteryVolts || delta.full) {
    slot.reading.batteryVolts = delta.hasBatteryVolts ? delta.batteryVolts : 0;
    slot.reading.hasBatteryVolts = delta.hasBatteryVolts || delta.full;
  }
  slot.reading.lastMs = nowMs;
  markDirty();
}

void ShellySmokeHub::ingest(const char* json, uint32_t nowMs) {
  expireListen(nowMs);
  ShellySmokeNotifyDelta delta = {};
  if (!ShellySmokeNotifyParser::parse(json, delta) || !delta.ok) {
    return;
  }
  int index = findById(delta.id);
  const bool knownAuthorized = index >= 0 && slots_[static_cast<size_t>(index)].authorized;
  if (!knownAuthorized && !listenOpen_) {
    return;
  }
  if (index < 0) {
    index = findFreeSlot();
    if (index < 0) {
      return;
    }
    slots_[static_cast<size_t>(index)] = {};
    slots_[static_cast<size_t>(index)].used = true;
    util::copyCString(slots_[static_cast<size_t>(index)].identity.id,
                      sizeof(slots_[static_cast<size_t>(index)].identity.id), delta.id);
    ShellyDisplayName::shortFromId(delta.id, slots_[static_cast<size_t>(index)].identity.name,
                                   sizeof(slots_[static_cast<size_t>(index)].identity.name));
  }
  applyDelta(slots_[static_cast<size_t>(index)], delta, nowMs);
}

void ShellySmokeHub::tick(uint32_t nowMs) {
  expireListen(nowMs);
  for (size_t i = 0; i < kMaxShellySmoke; ++i) {
    if (!slots_[i].used) {
      continue;
    }
    if (!slots_[i].authorized && slots_[i].reading.lastMs != 0 &&
        static_cast<uint32_t>(nowMs - slots_[i].reading.lastMs) > kShellySmokePostingTtlMs) {
      slots_[i] = {};
    }
  }
}

}  // namespace wattcycle::shelly
