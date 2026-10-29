#pragma once

#include "Auth/AuthTypes.h"

namespace wattcycle::auth {

/// NVS / flash backing for the salted login record.
class ICredentialStore {
 public:
  virtual ~ICredentialStore() = default;

  /// True when a username/password hash is stored.
  virtual bool hasCredentials() const = 0;
  /// Read the stored credential record.
  virtual bool load(StoredCredentials& out) const = 0;
  /// Persist a new credential record.
  virtual bool save(const StoredCredentials& credentials) = 0;
  /// Wipe stored credentials (after a confirmed reset).
  virtual void clear() = 0;
};

}  // namespace wattcycle::auth
