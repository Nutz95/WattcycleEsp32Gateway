#include "EcoFlow/Crypto/EcoFlowAuthToken.h"

#include <Arduino.h>
#include <MD5Builder.h>

namespace ecoflow::crypto {
namespace {

void toUpperHex(const uint8_t* digest, char outHex32[EcoFlowAuthToken::kHexBytes]) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  for (size_t i = 0; i < EcoFlowAuthToken::kDigestBytes; ++i) {
    outHex32[i * 2] = kHex[(digest[i] >> 4) & 0x0F];
    outHex32[i * 2 + 1] = kHex[digest[i] & 0x0F];
  }
}

}  // namespace

bool EcoFlowAuthToken::buildUpperHexMd5(const char* userId, const char* serial,
                                        char outHex32[kHexBytes]) {
  if (userId == nullptr || serial == nullptr || userId[0] == '\0' || serial[0] == '\0' ||
      outHex32 == nullptr) {
    return false;
  }

  MD5Builder md5;
  md5.begin();
  md5.add(String(userId) + String(serial));
  md5.calculate();
  uint8_t digest[kDigestBytes];
  md5.getBytes(digest);
  toUpperHex(digest, outHex32);
  return true;
}

}  // namespace ecoflow::crypto
