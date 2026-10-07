#include <unity.h>

#include "Auth/CredentialPolicy.h"

using wattcycle::auth::CredentialPolicy;

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_username_accepts_safe_chars() {
  char error[32] = {};
  TEST_ASSERT_TRUE(CredentialPolicy::validateUsername("alice_01", error, sizeof(error)));
  TEST_ASSERT_EQUAL_STRING("", error);
}

void test_username_rejects_specials() {
  char error[32] = {};
  TEST_ASSERT_FALSE(CredentialPolicy::validateUsername("a<script>", error, sizeof(error)));
  TEST_ASSERT_EQUAL_STRING("invalid_username", error);
}

void test_password_length_bounds() {
  char error[32] = {};
  TEST_ASSERT_FALSE(CredentialPolicy::validatePassword("short", error, sizeof(error)));
  TEST_ASSERT_EQUAL_STRING("password_too_short", error);

  char longPass[70];
  for (int i = 0; i < 69; ++i) {
    longPass[i] = 'a';
  }
  longPass[69] = '\0';
  TEST_ASSERT_FALSE(CredentialPolicy::validatePassword(longPass, error, sizeof(error)));
  TEST_ASSERT_EQUAL_STRING("password_too_long", error);
}

void test_body_rejects_oversized_and_nul() {
  char error[32] = {};
  char body[600];
  for (size_t i = 0; i < sizeof(body); ++i) {
    body[i] = 'x';
  }
  TEST_ASSERT_FALSE(
      CredentialPolicy::validateAuthJsonBody(body, sizeof(body), error, sizeof(error)));
  TEST_ASSERT_EQUAL_STRING("payload_too_large", error);

  const char withNul[] = {'{', '"', 'a', '"', ':', '"', 'b', '\0', '"', '}'};
  TEST_ASSERT_FALSE(CredentialPolicy::validateAuthJsonBody(withNul, sizeof(withNul), error,
                                                           sizeof(error)));
  TEST_ASSERT_EQUAL_STRING("invalid_json", error);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_username_accepts_safe_chars);
  RUN_TEST(test_username_rejects_specials);
  RUN_TEST(test_password_length_bounds);
  RUN_TEST(test_body_rejects_oversized_and_nul);
  return UNITY_END();
}
