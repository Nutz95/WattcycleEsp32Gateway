#pragma once

#include "Auth/IAuthSessionService.h"

#include <WebServer.h>

#include <cstddef>

namespace wattcycle::web {

/// Shared helpers for Shelly plug/smoke HTTP admin routes.
class ShellyHttpSupport {
 public:
  /// Copy the POST plain body into `body`. False when missing or too large.
  static bool copyPlainBody(WebServer& server, char* body, size_t capacity);
  /// Append JSON-escaped text into `out` at `used`.
  static void appendEscaped(char* out, size_t capacity, size_t& used, const char* text);
  /// Read `wg_session` cookie into `out`.
  static void collectSessionToken(WebServer& server, char* out, size_t capacity);
  /// True when the session cookie is authenticated.
  static bool requireAuth(WebServer& server, auth::IAuthSessionService& sessions);
  /// Send a JSON response with Cache-Control: no-store.
  static void sendJson(WebServer& server, int code, const char* json);
};

}  // namespace wattcycle::web
