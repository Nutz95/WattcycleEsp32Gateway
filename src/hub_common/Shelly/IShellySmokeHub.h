#pragma once

#include "Shelly/ShellySmokeReading.h"

#include <cstddef>
#include <cstdint>

namespace wattcycle::shelly {

/// Whitelist, live readings, and posting discovery for Shelly Plus Smoke.
class IShellySmokeHub {
 public:
  virtual ~IShellySmokeHub() = default;

  /// True while listen mode accepts unknown device ids.
  virtual bool listenOpen() const = 0;
  /// Open or close listen mode; `nowMs` starts the listen window.
  virtual void setListen(bool open, uint32_t nowMs) = 0;
  /// Add `id` to the whitelist with an optional display name.
  virtual bool authorize(const char* id, const char* name) = 0;
  /// Change the display name of an authorized detector.
  virtual bool rename(const char* id, const char* name) = 0;
  /// Remove `id` from the whitelist.
  virtual bool revoke(const char* id) = 0;
  /// Copy current display name for an authorized id.
  virtual bool copyName(const char* id, char* out, size_t capacity) const = 0;

  /// Copy authorized readings into `out`. Returns count written.
  virtual size_t snapshotAuthorized(ShellySmokeReading* out, size_t capacity,
                                    uint32_t nowMs) const = 0;
  /// Copy recent unauthorized posters into `out`. Returns count written.
  virtual size_t snapshotPosting(ShellySmokeReading* out, size_t capacity,
                                 uint32_t nowMs) const = 0;
  /// True once after ingest/authorize/rename/revoke/tick changed visible state.
  virtual bool consumeDirty() = 0;

  /// Ingest one UDP JSON datagram.
  virtual void ingest(const char* json, uint32_t nowMs) = 0;
  /// Expire listen and drop stale posters.
  virtual void tick(uint32_t nowMs) = 0;
};

}  // namespace wattcycle::shelly
