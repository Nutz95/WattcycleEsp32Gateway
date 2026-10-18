#include "Auth/NvsCredentialStore.h"

#include "Util/SafeCopy.h"

#include <cstring>

#include <Preferences.h>

namespace wattcycle::auth {
namespace {
constexpr const char* kNamespace = "wg_auth";
constexpr const char* kKeyUser = "user";
constexpr const char* kKeySalt = "salt";
constexpr const char* kKeyHash = "hash";
}  // namespace

bool NvsCredentialStore::hasCredentials() const {
  // Read-write open creates the namespace if missing (avoids Preferences spam).
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  const bool present = prefs.isKey(kKeyUser) && prefs.isKey(kKeySalt) && prefs.isKey(kKeyHash);
  prefs.end();
  return present;
}

bool NvsCredentialStore::load(StoredCredentials& out) const {
  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    return false;
  }
  const String user = prefs.getString(kKeyUser, "");
  const size_t saltLen = prefs.getBytesLength(kKeySalt);
  const size_t hashLen = prefs.getBytesLength(kKeyHash);
  if (user.length() == 0 || saltLen != kSaltBytes || hashLen != kHashBytes) {
    prefs.end();
    return false;
  }
  std::memset(&out, 0, sizeof(out));
  wattcycle::util::copyCString(out.username, sizeof(out.username), user.c_str());
  prefs.getBytes(kKeySalt, out.salt, kSaltBytes);
  prefs.getBytes(kKeyHash, out.hash, kHashBytes);
  prefs.end();
  return true;
}

bool NvsCredentialStore::save(const StoredCredentials& credentials) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  const bool ok = prefs.putString(kKeyUser, credentials.username) > 0 &&
                  prefs.putBytes(kKeySalt, credentials.salt, kSaltBytes) == kSaltBytes &&
                  prefs.putBytes(kKeyHash, credentials.hash, kHashBytes) == kHashBytes;
  prefs.end();
  return ok;
}

void NvsCredentialStore::clear() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return;
  }
  prefs.clear();
  prefs.end();
}

}  // namespace wattcycle::auth
