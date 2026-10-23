#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Shelly/NullShellyPlugConfigStore.h"
#include "Shelly/ShellyNotifyParser.h"
#include "Shelly/ShellyPlugHub.h"
#include "Shelly/ShellyPlugReading.h"

#include <cstdio>
#include <cstring>
#include <ctime>

namespace {

const char* kStatus = R"({
  "src": "shellyplugmg3-70af09e16164",
  "dst": "user_1",
  "method": "NotifyStatus",
  "params": {
    "ts": 1623243781.01,
    "switch:0": {
      "output": true,
      "apower": 12.4,
      "voltage": 230.1,
      "current": 0.054,
      "aenergy": { "total": 291.435 }
    }
  }
})";

const char* kPartial = R"({
  "src": "shellyplugmg3-70af09e16164",
  "method": "NotifyStatus",
  "params": { "switch:0": { "apower": 40.0 } }
})";

}  // namespace

void test_parse_full_status_fields() {
  wattcycle::shelly::ShellyNotifyDelta delta = {};
  TEST_ASSERT_TRUE(wattcycle::shelly::ShellyNotifyParser::parse(kStatus, delta));
  TEST_ASSERT_TRUE(delta.ok);
  TEST_ASSERT_EQUAL_STRING("shellyplugmg3-70af09e16164", delta.id);
  TEST_ASSERT_TRUE(delta.hasApower);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 12.4f, delta.apower);
  TEST_ASSERT_TRUE(delta.hasAenergy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 291.435f, delta.aenergyWh);
  TEST_ASSERT_TRUE(delta.hasOutput);
  TEST_ASSERT_TRUE(delta.output);
}

void test_parse_rejects_bad_method() {
  wattcycle::shelly::ShellyNotifyDelta delta = {};
  TEST_ASSERT_FALSE(wattcycle::shelly::ShellyNotifyParser::parse(
      "{\"src\":\"shellyplugmg3-aa\",\"method\":\"Nope\"}", delta));
}

void test_hub_ignores_unknown_without_listen() {
  wattcycle::shelly::NullShellyPlugConfigStore store;
  wattcycle::shelly::ShellyPlugHub hub(&store);
  hub.begin();
  hub.ingest(kStatus, 1000, 1700000000);
  wattcycle::shelly::ShellyPlugReading out[8] = {};
  TEST_ASSERT_EQUAL_UINT32(0, hub.snapshotAuthorized(out, 8, 1000));
  TEST_ASSERT_EQUAL_UINT32(0, hub.snapshotPosting(out, 8, 1000));
}

void test_hub_listen_authorize_rename_day_delta() {
  wattcycle::shelly::NullShellyPlugConfigStore store;
  wattcycle::shelly::ShellyPlugHub hub(&store);
  hub.begin();
  hub.setListen(true, 1000);
  hub.ingest(kStatus, 1000, 1700000000);
  wattcycle::shelly::ShellyPlugReading out[8] = {};
  TEST_ASSERT_EQUAL_UINT32(1, hub.snapshotPosting(out, 8, 1000));
  TEST_ASSERT_EQUAL_UINT32(0, hub.snapshotAuthorized(out, 8, 1000));
  TEST_ASSERT_TRUE(hub.authorize("shellyplugmg3-70af09e16164", "Frigo"));
  TEST_ASSERT_EQUAL_UINT32(1, hub.snapshotAuthorized(out, 8, 1000));
  TEST_ASSERT_EQUAL_UINT32(0, hub.snapshotPosting(out, 8, 1000));
  TEST_ASSERT_EQUAL_STRING("Frigo", out[0].name);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 12.4f, out[0].watts);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, out[0].whToday);

  hub.ingest(kPartial, 2000, 1700000000);
  TEST_ASSERT_EQUAL_UINT32(1, hub.snapshotAuthorized(out, 8, 2000));
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 40.0f, out[0].watts);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 291.435f, out[0].whTotal);

  TEST_ASSERT_TRUE(hub.rename("shellyplugmg3-70af09e16164", "Cave"));
  TEST_ASSERT_EQUAL_UINT32(1, hub.snapshotAuthorized(out, 8, 2000));
  TEST_ASSERT_EQUAL_STRING("Cave", out[0].name);
  TEST_ASSERT_TRUE(hub.consumeDirty());
  TEST_ASSERT_FALSE(hub.consumeDirty());

  // Same UTC day: aenergy 300 vs baseline 291.435 => ~8.565 Wh today.
  const char* sameDay = R"({
    "src": "shellyplugmg3-70af09e16164",
    "method": "NotifyStatus",
    "params": { "switch:0": { "aenergy": { "total": 300.0 } } }
  })";
  hub.ingest(sameDay, 3000, 1700000000);
  TEST_ASSERT_EQUAL_UINT32(1, hub.snapshotAuthorized(out, 8, 3000));
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 8.565f, out[0].whToday);

  // Next UTC day: baseline rolls to 300, then total 310 => 10 Wh today.
  const char* nextDay = R"({
    "src": "shellyplugmg3-70af09e16164",
    "method": "NotifyStatus",
    "params": { "switch:0": { "aenergy": { "total": 310.0 } } }
  })";
  const std::time_t nextEpoch = 1700000000 + 2 * 86400;
  hub.tick(4000, nextEpoch);
  hub.ingest(nextDay, 5000, nextEpoch);
  TEST_ASSERT_EQUAL_UINT32(1, hub.snapshotAuthorized(out, 8, 5000));
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 10.0f, out[0].whToday);
}

void test_hub_strips_comma_from_name() {
  wattcycle::shelly::NullShellyPlugConfigStore store;
  wattcycle::shelly::ShellyPlugHub hub(&store);
  hub.begin();
  hub.setListen(true, 1);
  hub.ingest(kStatus, 1, 1700000000);
  TEST_ASSERT_TRUE(hub.authorize("shellyplugmg3-70af09e16164", "fridge,kitchen"));
  char name[32] = {};
  TEST_ASSERT_TRUE(hub.copyName("shellyplugmg3-70af09e16164", name, sizeof(name)));
  TEST_ASSERT_EQUAL_STRING("fridgekitchen", name);
  TEST_ASSERT_FALSE(hub.rename("shellyplugmg3-70af09e16164", ",,,"));
}

void test_hub_rejects_overflow_authorize() {
  wattcycle::shelly::NullShellyPlugConfigStore store;
  wattcycle::shelly::ShellyPlugHub hub(&store);
  hub.begin();
  hub.setListen(true, 1);
  for (int i = 0; i < 8; ++i) {
    char id[48] = {};
    std::snprintf(id, sizeof(id), "shellyplugmg3-%012d", i);
    char frame[160] = {};
    std::snprintf(frame, sizeof(frame),
                  "{\"src\":\"%s\",\"method\":\"NotifyStatus\",\"params\":{\"switch:0\":{\"apower\":1}}}",
                  id);
    hub.ingest(frame, static_cast<uint32_t>(i + 1), 1700000000);
    TEST_ASSERT_TRUE(hub.authorize(id, "n"));
  }
  TEST_ASSERT_FALSE(hub.authorize("shellyplugmg3-999999999999", "x"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_full_status_fields);
  RUN_TEST(test_parse_rejects_bad_method);
  RUN_TEST(test_hub_ignores_unknown_without_listen);
  RUN_TEST(test_hub_listen_authorize_rename_day_delta);
  RUN_TEST(test_hub_strips_comma_from_name);
  RUN_TEST(test_hub_rejects_overflow_authorize);
  return UNITY_END();
}
