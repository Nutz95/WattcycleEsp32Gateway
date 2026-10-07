#include "Auth/SessionStore.h"

#include "Util/RandomBytes.h"
#include "Util/SafeCopy.h"

#include <cstdio>
#include <cstring>

namespace wattcycle::auth {
namespace {

void bytesToHex(const uint8_t* bytes, size_t length, char* out, size_t outCapacity) {
  if (outCapacity < length * 2 + 1) {
    if (outCapacity > 0) {
      out[0] = '\0';
    }
    return;
  }
  for (size_t i = 0; i < length; ++i) {
    std::snprintf(out + i * 2, 3, "%02x", bytes[i]);
  }
  out[length * 2] = '\0';
}

}  // namespace

void SessionStore::clear() {
  for (auto& session : sessions_) {
    session = {};
  }
}

bool SessionStore::create(char* tokenOut, size_t tokenCapacity, uint32_t nowMs) {
  if (tokenOut == nullptr || tokenCapacity < kSessionTokenHexLen + 1) {
    return false;
  }
  int slot = -1;
  for (size_t i = 0; i < kMaxSessions; ++i) {
    if (!sessions_[i].used || sessions_[i].expiresAtMs <= nowMs) {
      slot = static_cast<int>(i);
      break;
    }
  }
  if (slot < 0) {
    slot = 0;  // reuse oldest slot under pressure
  }

  uint8_t raw[kSessionTokenBytes] = {};
  util::fillRandom(raw, sizeof(raw));
  bytesToHex(raw, sizeof(raw), sessions_[slot].tokenHex, sizeof(sessions_[slot].tokenHex));
  sessions_[slot].used = true;
  sessions_[slot].expiresAtMs = nowMs + kSessionTtlMs;
  wattcycle::util::copyCString(tokenOut, tokenCapacity, sessions_[slot].tokenHex);
  return true;
}

bool SessionStore::valid(const char* token, uint32_t nowMs) const {
  if (token == nullptr || token[0] == '\0') {
    return false;
  }
  for (const auto& session : sessions_) {
    if (!session.used) {
      continue;
    }
    if (session.expiresAtMs <= nowMs) {
      continue;
    }
    if (std::strcmp(session.tokenHex, token) == 0) {
      return true;
    }
  }
  return false;
}

void SessionStore::revoke(const char* token) {
  if (token == nullptr) {
    return;
  }
  for (auto& session : sessions_) {
    if (session.used && std::strcmp(session.tokenHex, token) == 0) {
      session = {};
      return;
    }
  }
}

}  // namespace wattcycle::auth
