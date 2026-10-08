#include "Auth/NvsCredentialStore.h"

#include "Util/SafeCopy.h"

#include <cstring>

#ifndef UNIT_TEST
#include <Preferences.h>
#endif

namespace wattcycle::auth {
namespace {
#ifndef UNIT_TEST
constexpr const char* kNamespace = "wg_auth";
constexpr const char* kKeyUser = "user";
constexpr const char* kKeySalt = "salt";
constexpr const char* kKeyHash = "hash";
#endif
}  // namespace

bool NvsCredentialStore::hasCredentials() const {
  StoredCredentials credentials;
  return load(credentials);
}

bool NvsCredentialStore::load(StoredCredentials& out) const {
#ifndef UNIT_TEST
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
#else
  (void)out;
  return false;
#endif
}

bool NvsCredentialStore::save(const StoredCredentials& credentials) {
#ifndef UNIT_TEST
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  const bool ok = prefs.putString(kKeyUser, credentials.username) > 0 &&
                  prefs.putBytes(kKeySalt, credentials.salt, kSaltBytes) == kSaltBytes &&
                  prefs.putBytes(kKeyHash, credentials.hash, kHashBytes) == kHashBytes;
  prefs.end();
  return ok;
#else
  (void)credentials;
  return false;
#endif
}

void NvsCredentialStore::clear() {
#ifndef UNIT_TEST
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return;
  }
  prefs.clear();
  prefs.end();
#endif
}

}  // namespace wattcycle::auth
