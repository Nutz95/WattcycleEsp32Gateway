#pragma once

#include "Auth/AuthTypes.h"

namespace wattcycle::auth {

class ICredentialStore {
 public:
  virtual ~ICredentialStore() = default;

  virtual bool hasCredentials() const = 0;
  virtual bool load(StoredCredentials& out) const = 0;
  virtual bool save(const StoredCredentials& credentials) = 0;
  virtual void clear() = 0;
};

}  // namespace wattcycle::auth
