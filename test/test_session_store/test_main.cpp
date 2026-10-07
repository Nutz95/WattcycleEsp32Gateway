#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Auth/SessionStore.h"

#include <cstring>

using wattcycle::auth::SessionStore;
using wattcycle::auth::kSessionTokenHexLen;
using wattcycle::auth::kSessionTtlMs;

void test_create_and_validate() {
  SessionStore store;
  char token[kSessionTokenHexLen + 1] = {};
  TEST_ASSERT_TRUE(store.create(token, sizeof(token), 1000));
  TEST_ASSERT_TRUE(token[0] != '\0');
  TEST_ASSERT_TRUE(store.valid(token, 1000));
  TEST_ASSERT_FALSE(store.valid(token, 1000 + kSessionTtlMs + 1));
}

void test_evicts_oldest_when_full() {
  SessionStore store;
  char tokens[SessionStore::kMaxSessions + 1][kSessionTokenHexLen + 1] = {};
  for (size_t i = 0; i < SessionStore::kMaxSessions; ++i) {
    const uint32_t nowMs = static_cast<uint32_t>(1000 + i * 1000);
    TEST_ASSERT_TRUE(store.create(tokens[i], sizeof(tokens[i]), nowMs));
  }
  // All slots full; next create should evict the earliest-expiring (first) session.
  TEST_ASSERT_TRUE(store.create(tokens[SessionStore::kMaxSessions],
                                sizeof(tokens[SessionStore::kMaxSessions]), 9000));
  TEST_ASSERT_FALSE(store.valid(tokens[0], 9000));
  TEST_ASSERT_TRUE(store.valid(tokens[SessionStore::kMaxSessions], 9000));
  for (size_t i = 1; i < SessionStore::kMaxSessions; ++i) {
    TEST_ASSERT_TRUE(store.valid(tokens[i], 9000));
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_create_and_validate);
  RUN_TEST(test_evicts_oldest_when_full);
  return UNITY_END();
}
