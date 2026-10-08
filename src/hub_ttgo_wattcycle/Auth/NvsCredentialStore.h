#pragma once

#include "Auth/ICredentialStore.h"

namespace wattcycle::auth {

class NvsCredentialStore : public ICredentialStore {
 public:
  bool hasCredentials() const override;
  bool load(StoredCredentials& out) const override;
  bool save(const StoredCredentials& credentials) override;
  void clear() override;
};

}  // namespace wattcycle::auth
