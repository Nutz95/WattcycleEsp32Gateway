#pragma once

#include "Shelly/ShellyPlugReading.h"

#include <cstddef>
#include <cstdint>
#include <ctime>

namespace wattcycle::shelly {

/// Whitelist, live readings, and posting discovery for Shelly plugs.
class IShellyPlugHub {
 public:
  virtual ~IShellyPlugHub() = default;

  virtual bool listenOpen() const = 0;
  virtual void setListen(bool open, uint32_t nowMs) = 0;
  virtual bool authorize(const char* id, const char* name) = 0;
  virtual bool rename(const char* id, const char* name) = 0;
  virtual bool revoke(const char* id) = 0;
  /// Copy current display name for an authorized id.
  virtual bool copyName(const char* id, char* out, size_t capacity) const = 0;

  /// Copy authorized readings into `out`. Returns count written.
  virtual size_t snapshotAuthorized(ShellyPlugReading* out, size_t capacity,
                                    uint32_t nowMs) const = 0;
  /// Copy recent unauthorized posters into `out`. Returns count written.
  virtual size_t snapshotPosting(ShellyPlugReading* out, size_t capacity,
                                 uint32_t nowMs) const = 0;
  /// True once after ingest/authorize/rename/revoke/tick changed visible state.
  virtual bool consumeDirty() = 0;

  /// Ingest one UDP JSON datagram. `epochUtc` may be 0 when NTP is down.
  virtual void ingest(const char* json, uint32_t nowMs, std::time_t epochUtc) = 0;
  virtual void tick(uint32_t nowMs, std::time_t epochUtc) = 0;
};

}  // namespace wattcycle::shelly
