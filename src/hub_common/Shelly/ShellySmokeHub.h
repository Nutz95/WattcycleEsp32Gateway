#pragma once

#include "Shelly/IShellySmokeConfigStore.h"
#include "Shelly/IShellySmokeHub.h"
#include "Shelly/ShellySmokeNotifyDelta.h"
#include "Shelly/ShellySmokeSlot.h"

namespace wattcycle::shelly {

/// In-memory whitelist and smoke readings. Optional config store for SD.
class ShellySmokeHub : public IShellySmokeHub {
 public:
  explicit ShellySmokeHub(IShellySmokeConfigStore* store = nullptr);

  void begin();

  bool listenOpen() const override;
  void setListen(bool open, uint32_t nowMs) override;
  bool authorize(const char* id, const char* name) override;
  bool rename(const char* id, const char* name) override;
  bool revoke(const char* id) override;
  bool copyName(const char* id, char* out, size_t capacity) const override;

  size_t snapshotAuthorized(ShellySmokeReading* out, size_t capacity,
                            uint32_t nowMs) const override;
  size_t snapshotPosting(ShellySmokeReading* out, size_t capacity, uint32_t nowMs) const override;
  bool consumeDirty() override;

  void ingest(const char* json, uint32_t nowMs) override;
  void tick(uint32_t nowMs) override;

 private:
  int findById(const char* id) const;
  int findFreeSlot() const;
  bool persistAuthorized();
  void expireListen(uint32_t nowMs);
  void markDirty();
  void applyDelta(ShellySmokeSlot& slot, const ShellySmokeNotifyDelta& delta, uint32_t nowMs);
  void fillReading(const ShellySmokeSlot& slot, ShellySmokeReading& out, uint32_t nowMs) const;

  IShellySmokeConfigStore* store_;
  ShellySmokeSlot slots_[kMaxShellySmoke] = {};
  bool listenOpen_ = false;
  uint32_t listenUntilMs_ = 0;
  bool dirty_ = false;
};

}  // namespace wattcycle::shelly
