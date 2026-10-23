#include "Shelly/ShellyPlugHub.h"

#include "Shelly/ShellyNotifyParser.h"
#include "Shelly/ShellyPlugId.h"
#include "Util/SafeCopy.h"

#include <ctime>
#include <cstring>

namespace wattcycle::shelly {

ShellyPlugHub::ShellyPlugHub(IShellyPlugConfigStore* store) : store_(store) {}

void ShellyPlugHub::begin() {
  for (size_t i = 0; i < kMaxShellyPlugs; ++i) {
    slots_[i] = {};
  }
  if (store_ == nullptr) {
    return;
  }
  ShellyPlugIdentity loaded[kMaxShellyPlugs] = {};
  const size_t count = store_->loadAuthorized(loaded, kMaxShellyPlugs);
  size_t slot = 0;
  for (size_t i = 0; i < count && slot < kMaxShellyPlugs; ++i) {
    if (!shellyPlugIdValid(loaded[i].id)) {
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
  float baselines[kMaxShellyPlugs] = {};
  char ids[kMaxShellyPlugs][kShellyIdCapacity] = {};
  size_t dayCount = 0;
  uint32_t day = 0;
  if (!store_->loadDayBaseline(day, baselines, ids, kMaxShellyPlugs, dayCount)) {
    return;
  }
  dayYyyymmdd_ = day;
  for (size_t i = 0; i < dayCount; ++i) {
    const int found = findById(ids[i]);
    if (found < 0) {
      continue;
    }
    slots_[static_cast<size_t>(found)].dayBaselineWh = baselines[i];
    slots_[static_cast<size_t>(found)].hasBaseline = true;
  }
}

bool ShellyPlugHub::listenOpen() const {
  return listenOpen_;
}

void ShellyPlugHub::setListen(bool open, uint32_t nowMs) {
  listenOpen_ = open;
  listenUntilMs_ = open ? nowMs + kShellyListenMs : 0;
  markDirty();
}

void ShellyPlugHub::expireListen(uint32_t nowMs) {
  if (listenOpen_ && listenUntilMs_ != 0 && static_cast<int32_t>(nowMs - listenUntilMs_) >= 0) {
    listenOpen_ = false;
    listenUntilMs_ = 0;
    markDirty();
  }
}

uint32_t ShellyPlugHub::dayKey(std::time_t epochUtc) const {
  if (epochUtc <= 0) {
    return 0;
  }
  const std::tm* utc = std::gmtime(&epochUtc);
  if (utc == nullptr) {
    return 0;
  }
  return static_cast<uint32_t>((utc->tm_year + 1900) * 10000 + (utc->tm_mon + 1) * 100 +
                               utc->tm_mday);
}

void ShellyPlugHub::rollDayIfNeeded(std::time_t epochUtc) {
  if (epochUtc <= 0) {
    return;
  }
  const uint32_t today = dayKey(epochUtc);
  if (today == 0) {
    return;
  }
  if (dayYyyymmdd_ == 0) {
    dayYyyymmdd_ = today;
    return;
  }
  if (today == dayYyyymmdd_) {
    return;
  }
  dayYyyymmdd_ = today;
  for (size_t i = 0; i < kMaxShellyPlugs; ++i) {
    if (!slots_[i].used || !slots_[i].authorized || !slots_[i].reading.hasWhTotal) {
      continue;
    }
    slots_[i].dayBaselineWh = slots_[i].reading.whTotal;
    slots_[i].hasBaseline = true;
    slots_[i].reading.whToday = 0;
  }
  persistDay();
}

void ShellyPlugHub::refreshToday(ShellyPlugSlot& slot, std::time_t epochUtc) {
  if (!slot.authorized || !slot.reading.hasWhTotal || epochUtc <= 0) {
    return;
  }
  rollDayIfNeeded(epochUtc);
  if (!slot.hasBaseline) {
    slot.dayBaselineWh = slot.reading.whTotal;
    slot.hasBaseline = true;
    persistDay();
  }
  if (slot.reading.whTotal < slot.dayBaselineWh) {
    slot.dayBaselineWh = slot.reading.whTotal;
    persistDay();
  }
  slot.reading.whToday = slot.reading.whTotal - slot.dayBaselineWh;
  if (slot.reading.whToday < 0) {
    slot.reading.whToday = 0;
  }
}

void ShellyPlugHub::applyDelta(ShellyPlugSlot& slot, const ShellyNotifyDelta& delta, uint32_t nowMs,
                               std::time_t epochUtc) {
  util::copyCString(slot.identity.id, sizeof(slot.identity.id), delta.id);
  util::copyCString(slot.reading.id, sizeof(slot.reading.id), delta.id);
  if (delta.hasApower || delta.full) {
    slot.reading.watts = delta.hasApower ? delta.apower : 0;
    slot.reading.hasWatts = delta.hasApower || delta.full;
  }
  if (delta.hasVoltage || delta.full) {
    slot.reading.volts = delta.hasVoltage ? delta.voltage : 0;
    slot.reading.hasVolts = delta.hasVoltage || delta.full;
  }
  if (delta.hasCurrent || delta.full) {
    slot.reading.amps = delta.hasCurrent ? delta.current : 0;
    slot.reading.hasAmps = delta.hasCurrent || delta.full;
  }
  if (delta.hasAenergy || delta.full) {
    slot.reading.whTotal = delta.hasAenergy ? delta.aenergyWh : 0;
    slot.reading.hasWhTotal = delta.hasAenergy || delta.full;
  }
  if (delta.hasOutput || delta.full) {
    slot.reading.output = delta.hasOutput ? delta.output : false;
    slot.reading.hasOutput = delta.hasOutput || delta.full;
  }
  slot.reading.lastMs = nowMs;
  refreshToday(slot, epochUtc);
  markDirty();
}

void ShellyPlugHub::ingest(const char* json, uint32_t nowMs, std::time_t epochUtc) {
  expireListen(nowMs);
  ShellyNotifyDelta delta = {};
  if (!ShellyNotifyParser::parse(json, delta) || !delta.ok) {
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
    shortNameFromId(delta.id, slots_[static_cast<size_t>(index)].identity.name,
                    sizeof(slots_[static_cast<size_t>(index)].identity.name));
  }
  applyDelta(slots_[static_cast<size_t>(index)], delta, nowMs, epochUtc);
}

void ShellyPlugHub::tick(uint32_t nowMs, std::time_t epochUtc) {
  expireListen(nowMs);
  rollDayIfNeeded(epochUtc);
  for (size_t i = 0; i < kMaxShellyPlugs; ++i) {
    if (!slots_[i].used) {
      continue;
    }
    refreshToday(slots_[i], epochUtc);
    if (!slots_[i].authorized && slots_[i].reading.lastMs != 0 &&
        static_cast<uint32_t>(nowMs - slots_[i].reading.lastMs) > kShellyPostingTtlMs) {
      slots_[i] = {};
    }
  }
}

}  // namespace wattcycle::shelly
