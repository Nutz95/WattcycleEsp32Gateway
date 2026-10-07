#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Auth/PasswordHasher.h"
#include "Auth/Sha256.h"
#include "Util/RandomBytes.h"

#include <cstring>

using wattcycle::auth::PasswordHasher;
using wattcycle::auth::Sha256;
using wattcycle::auth::kHashBytes;
using wattcycle::auth::kSaltBytes;

void test_sha256_empty() {
  uint8_t out[32] = {};
  const uint8_t empty = 0;
  Sha256::hash(&empty, 0, out);
  // e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
  TEST_ASSERT_EQUAL_HEX8(0xe3, out[0]);
  TEST_ASSERT_EQUAL_HEX8(0xb0, out[1]);
  TEST_ASSERT_EQUAL_HEX8(0xc4, out[2]);
  TEST_ASSERT_EQUAL_HEX8(0x42, out[3]);
  TEST_ASSERT_EQUAL_HEX8(0x55, out[31]);
}

void test_password_roundtrip() {
  uint8_t salt[kSaltBytes] = {};
  uint8_t hash[kHashBytes] = {};
  wattcycle::util::fillRandom(salt, sizeof(salt));
  TEST_ASSERT_TRUE(PasswordHasher::hashPassword("secret-pass", salt, hash));
  TEST_ASSERT_TRUE(PasswordHasher::verifyPassword("secret-pass", salt, hash));
  TEST_ASSERT_FALSE(PasswordHasher::verifyPassword("wrong-pass", salt, hash));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sha256_empty);
  RUN_TEST(test_password_roundtrip);
  return UNITY_END();
}
