#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Web/JsonField.h"

void test_extract_username_password() {
  const char* json = "{\"username\":\"alice\",\"password\":\"s3cret!!\"}";
  char user[32] = {};
  char pass[64] = {};
  TEST_ASSERT_TRUE(wattcycle::web::extractJsonStringField(json, "username", user, sizeof(user)));
  TEST_ASSERT_TRUE(wattcycle::web::extractJsonStringField(json, "password", pass, sizeof(pass)));
  TEST_ASSERT_EQUAL_STRING("alice", user);
  TEST_ASSERT_EQUAL_STRING("s3cret!!", pass);
}

void test_extract_missing_key() {
  char out[16] = {};
  TEST_ASSERT_FALSE(
      wattcycle::web::extractJsonStringField("{\"username\":\"a\"}", "password", out, sizeof(out)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_extract_username_password);
  RUN_TEST(test_extract_missing_key);
  return UNITY_END();
}
