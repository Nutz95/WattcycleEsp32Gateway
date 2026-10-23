#include "HttpApi/HttpReadDecision.h"

namespace wattcycle::web {

HttpReadDecision decideHttpRead(auth::IGuestAccess* guest, auth::IAuthSessionService& sessions,
                                const char* token) {
  HttpReadDecision decision;
  if (sessions.isAuthenticated(token)) {
    decision.signedIn = true;
    return decision;
  }
  if (guest != nullptr && guest->enabled()) {
    decision.guest = true;
    decision.mask = guest->tabs() & auth::IGuestAccess::kTabMask;
  }
  return decision;
}

}  // namespace wattcycle::web
