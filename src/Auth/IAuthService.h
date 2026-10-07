#pragma once

#include "Auth/IAuthPhysicalConfirm.h"
#include "Auth/IAuthSessionService.h"

namespace wattcycle::auth {

/// Combined port for composition root / concrete AuthService (implements both).
class IAuthService : public IAuthSessionService, public IAuthPhysicalConfirm {
 public:
  ~IAuthService() override = default;
};

}  // namespace wattcycle::auth
