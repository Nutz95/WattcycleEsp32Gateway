#pragma once

#include "Shelly/IShellyPlugConfigStore.h"
#include "Shelly/IShellyPlugHub.h"
#include "Shelly/ShellyNotifyDelta.h"
#include "Shelly/ShellyPlugLimits.h"
#include "Shelly/ShellyPlugSlot.h"

namespace wattcycle::shelly {

/// In-memory whitelist and readings. Optional config store for SD.
class ShellyPlugHub : public IShellyPlugHub {
 public:
  explicit ShellyPlugHub(IShellyPlugConfigStore* store = nullptr);

  void begin();

  bool listenOpen() const override;
  void setListen(bool open, uint32_t nowMs) override;
  bool authorize(const char* id, const char* name) override;
  bool rename(const char* id, const char* name) override;
  bool revoke(const char* id) override;
  bool copyName(const char* id, char* out, size_t capacity) const override;

  size_t snapshotAuthorized(ShellyPlugReading* out, size_t capacity,
                            uint32_t nowMs) const override;
  size_t snapshotPosting(ShellyPlugReading* out, size_t capacity, uint32_t nowMs) const override;
  bool consumeDirty() override;

  void ingest(const char* json, uint32_t nowMs, std::time_t epochUtc) override;
  void tick(uint32_t nowMs, std::time_t epochUtc) override;

 private:
  int findById(const char* id) const;
  int findFreeSlot() const;
  bool sanitizeName(const char* name, char* out, size_t capacity) const;
  bool shortNameFromId(const char* id, char* out, size_t capacity) const;
  bool persistAuthorized();
  bool persistDay();
  void expireListen(uint32_t nowMs);
  void markDirty();
  void applyDelta(ShellyPlugSlot& slot, const ShellyNotifyDelta& delta, uint32_t nowMs,
                   std::time_t epochUtc);
  void rollDayIfNeeded(std::time_t epochUtc);
  void refreshToday(ShellyPlugSlot& slot, std::time_t epochUtc);
  uint32_t dayKey(std::time_t epochUtc) const;
  void fillReading(const ShellyPlugSlot& slot, ShellyPlugReading& out, uint32_t nowMs) const;

  IShellyPlugConfigStore* store_;
  ShellyPlugSlot slots_[kMaxShellyPlugs] = {};
  bool listenOpen_ = false;
  uint32_t listenUntilMs_ = 0;
  uint32_t dayYyyymmdd_ = 0;
  bool dirty_ = false;
};

}  // namespace wattcycle::shelly
