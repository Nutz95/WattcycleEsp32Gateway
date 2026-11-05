#include "HttpApi/ShellyHttpSupport.h"

#include "Auth/AuthTypes.h"
#include "Util/SafeCopy.h"

#include <cstring>

namespace wattcycle::web {

bool ShellyHttpSupport::copyPlainBody(WebServer& server, char* body, size_t capacity) {
  if (!server.hasArg("plain")) {
    return false;
  }
  const String plain = server.arg("plain");
  if (plain.length() == 0 || plain.length() >= capacity) {
    return false;
  }
  std::memcpy(body, plain.c_str(), plain.length());
  body[plain.length()] = '\0';
  return true;
}

void ShellyHttpSupport::appendEscaped(char* out, size_t capacity, size_t& used, const char* text) {
  if (text == nullptr) {
    return;
  }
  for (const char* cursor = text; *cursor != '\0' && used + 2 < capacity; ++cursor) {
    const char ch = *cursor;
    if (ch == '"' || ch == '\\') {
      if (used + 3 >= capacity) {
        return;
      }
      out[used++] = '\\';
    }
    if (static_cast<unsigned char>(ch) < 32u) {
      continue;
    }
    out[used++] = ch;
  }
  out[used] = '\0';
}

void ShellyHttpSupport::collectSessionToken(WebServer& server, char* out, size_t capacity) {
  out[0] = '\0';
  if (!server.hasHeader("Cookie")) {
    return;
  }
  const String cookie = server.header("Cookie");
  const int idx = cookie.indexOf("wg_session=");
  if (idx < 0) {
    return;
  }
  int start = idx + 11;
  int end = cookie.indexOf(';', start);
  if (end < 0) {
    end = cookie.length();
  }
  util::copyCString(out, capacity, cookie.substring(start, end).c_str());
}

bool ShellyHttpSupport::requireAuth(WebServer& server, auth::IAuthSessionService& sessions) {
  char token[auth::kSessionTokenHexLen + 1] = {};
  collectSessionToken(server, token, sizeof(token));
  if (!sessions.isAuthenticated(token)) {
    sendJson(server, 401, "{\"error\":\"unauthorized\"}");
    return false;
  }
  return true;
}

void ShellyHttpSupport::sendJson(WebServer& server, int code, const char* json) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", json);
}

}  // namespace wattcycle::web
