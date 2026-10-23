#pragma once

#include "Auth/IAuthSessionService.h"
#include "Auth/IGuestAccess.h"

#include <cstdint>

namespace wattcycle::web {

/// One answer for a telemetry or history request: signed in, guest with a mask, or denied.
struct HttpReadDecision {
  bool signedIn = false;
  bool guest = false;
  uint32_t mask = 0;

  /// Signed-in session or an enabled guest.
  bool allowed() const { return signedIn || guest; }
  /// Signed-in callers pass every tab. A guest passes only bits saved in the mask.
  bool allows(uint32_t bit) const { return signedIn || (guest && (mask & bit) != 0); }
};

/// Session cookie wins. Otherwise an enabled guest receives the saved tab mask.
HttpReadDecision decideHttpRead(auth::IGuestAccess* guest, auth::IAuthSessionService& sessions,
                                const char* token);

}  // namespace wattcycle::web
