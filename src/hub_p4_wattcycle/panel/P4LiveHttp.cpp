#include "P4LiveHttp.h"

#include <cstdio>
#include <cstring>
#include <strings.h>

namespace wattcycle::panel {
namespace {

bool readHeaders(WiFiClient& client, char* buffer, size_t capacity) {
  size_t used = 0;
  const uint32_t deadline = millis() + 2000;
  buffer[0] = '\0';
  while (used + 1 < capacity && static_cast<int32_t>(deadline - millis()) > 0) {
    if (!client.connected()) {
      return false;
    }
    if (client.available() == 0) {
      delay(1);
      continue;
    }
    const int got = client.read(reinterpret_cast<uint8_t*>(buffer + used), capacity - 1 - used);
    if (got <= 0) {
      delay(1);
      continue;
    }
    used += static_cast<size_t>(got);
    buffer[used] = '\0';
    if (std::strstr(buffer, "\r\n\r\n") != nullptr) {
      return true;
    }
  }
  return false;
}

bool sessionAllowed(const char* headers, auth::IAuthSessionService& auth) {
  const char* marker = strcasestr(headers, "wg_session=");
  if (marker == nullptr) {
    return false;
  }
  marker += std::strlen("wg_session=");
  while (*marker == ' ') {
    ++marker;
  }
  char token[auth::kSessionTokenHexLen + 1] = {};
  size_t length = 0;
  while (marker[length] != '\0' && marker[length] != ';' && marker[length] != '\r' &&
         marker[length] != ' ' && length < auth::kSessionTokenHexLen) {
    token[length] = marker[length];
    ++length;
  }
  token[length] = '\0';
  return auth.isAuthenticated(token);
}

void copyOrigin(const char* headers, char* origin, size_t capacity) {
  if (origin == nullptr || capacity == 0) {
    return;
  }
  origin[0] = '\0';
  const char* marker = strcasestr(headers, "origin:");
  if (marker == nullptr) {
    return;
  }
  marker += 7;
  while (*marker == ' ') {
    ++marker;
  }
  size_t length = 0;
  while (marker[length] != '\0' && marker[length] != '\r' && length + 1 < capacity) {
    origin[length] = marker[length];
    ++length;
  }
  origin[length] = '\0';
}

}  // namespace

bool liveWriteAll(WiFiClient& client, const uint8_t* data, size_t length) {
  size_t sent = 0;
  while (sent < length) {
    if (!client.connected()) {
      return false;
    }
    const int wrote = client.write(data + sent, length - sent);
    if (wrote <= 0) {
      return false;
    }
    sent += static_cast<size_t>(wrote);
  }
  return true;
}

void liveSendStatus(WiFiClient& client, int code, const char* reason) {
  char head[96] = {};
  std::snprintf(head, sizeof(head), "HTTP/1.1 %d %s\r\nConnection: close\r\nContent-Length: 0\r\n\r\n",
                code, reason);
  liveWriteAll(client, reinterpret_cast<const uint8_t*>(head), std::strlen(head));
}

bool liveAcceptAuthorized(WiFiServer& server, WiFiClient& client, auth::IAuthSessionService& auth,
                          char* origin, size_t originCapacity) {
  if (client && client.connected()) {
    return true;
  }
  client.stop();
  WiFiClient incoming = server.accept();
  if (!incoming) {
    return false;
  }
  char headers[1536] = {};
  if (!readHeaders(incoming, headers, sizeof(headers)) || !sessionAllowed(headers, auth)) {
    liveSendStatus(incoming, 401, "unauthorized");
    incoming.stop();
    return false;
  }
  copyOrigin(headers, origin, originCapacity);
  client = incoming;
  return true;
}

void liveFillCors(const char* origin, char* out, size_t capacity) {
  if (out == nullptr || capacity == 0) {
    return;
  }
  out[0] = '\0';
  if (origin == nullptr || origin[0] == '\0') {
    return;
  }
  std::snprintf(out, capacity,
                "Access-Control-Allow-Origin: %s\r\nAccess-Control-Allow-Credentials: true\r\n", origin);
}

}  // namespace wattcycle::panel
