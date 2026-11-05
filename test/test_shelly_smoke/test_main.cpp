#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Shelly/NullShellySmokeConfigStore.h"
#include "Shelly/ShellySmokeHub.h"
#include "Shelly/ShellySmokeNotifyParser.h"
#include "Shelly/ShellySmokeReading.h"

#include <cstdio>
#include <cstring>

namespace {

const char* kStatus = R"({
  "src": "shellyplussmoke-aabbccddeeff",
  "dst": "user_1",
  "method": "NotifyStatus",
  "params": {
    "ts": 1623243781.01,
    "smoke:0": { "id": 0, "alarm": true, "mute": false },
    "devicepower:0": { "id": 0, "battery": { "V": 2.98, "percent": 87 } }
  }
})";

const char* kClear = R"({
  "src": "shellyplussmoke-aabbccddeeff",
  "method": "NotifyStatus",
  "params": { "smoke:0": { "alarm": false, "mute": true } }
})";

}  // namespace

void test_parse_smoke_status_fields() {
  wattcycle::shelly::ShellySmokeNotifyDelta delta = {};
  TEST_ASSERT_TRUE(wattcycle::shelly::ShellySmokeNotifyParser::parse(kStatus, delta));
  TEST_ASSERT_TRUE(delta.ok);
  TEST_ASSERT_EQUAL_STRING("shellyplussmoke-aabbccddeeff", delta.id);
  TEST_ASSERT_TRUE(delta.hasAlarm);
  TEST_ASSERT_TRUE(delta.alarm);
  TEST_ASSERT_TRUE(delta.hasMute);
  TEST_ASSERT_FALSE(delta.mute);
  TEST_ASSERT_TRUE(delta.hasBatteryPercent);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 87.0f, delta.batteryPercent);
  TEST_ASSERT_TRUE(delta.hasBatteryVolts);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.98f, delta.batteryVolts);
}

void test_reject_plug_id() {
  wattcycle::shelly::ShellySmokeNotifyDelta delta = {};
  TEST_ASSERT_FALSE(wattcycle::shelly::ShellySmokeNotifyParser::parse(
      "{\"src\":\"shellyplugmg3-aa\",\"method\":\"NotifyStatus\",\"params\":{\"alarm\":true}}",
      delta));
}

void test_authorize_and_ingest() {
  wattcycle::shelly::NullShellySmokeConfigStore store;
  wattcycle::shelly::ShellySmokeHub hub(&store);
  hub.begin();
  hub.setListen(true, 1000);
  hub.ingest(kStatus, 1000);
  wattcycle::shelly::ShellySmokeReading out[4] = {};
  TEST_ASSERT_EQUAL(1, hub.snapshotPosting(out, 4, 1000));
  TEST_ASSERT_TRUE(out[0].alarm);
  TEST_ASSERT_TRUE(hub.authorize("shellyplussmoke-aabbccddeeff", "Hall"));
  TEST_ASSERT_EQUAL(1, hub.snapshotAuthorized(out, 4, 1000));
  TEST_ASSERT_EQUAL_STRING("Hall", out[0].name);
  hub.ingest(kClear, 2000);
  TEST_ASSERT_EQUAL(1, hub.snapshotAuthorized(out, 4, 2000));
  TEST_ASSERT_FALSE(out[0].alarm);
  TEST_ASSERT_TRUE(out[0].mute);
}

void test_stale_still_online_within_window() {
  wattcycle::shelly::NullShellySmokeConfigStore store;
  wattcycle::shelly::ShellySmokeHub hub(&store);
  hub.begin();
  TEST_ASSERT_TRUE(hub.authorize("shellyplussmoke-aabbccddeeff", "Hall"));
  hub.ingest(kStatus, 1000);
  wattcycle::shelly::ShellySmokeReading out[4] = {};
  const uint32_t later = 1000u + (24u * 3600u * 1000u);
  TEST_ASSERT_EQUAL(1, hub.snapshotAuthorized(out, 4, later));
  TEST_ASSERT_TRUE(out[0].online);
}

void test_max_smoke_slots() {
  wattcycle::shelly::NullShellySmokeConfigStore store;
  wattcycle::shelly::ShellySmokeHub hub(&store);
  hub.begin();
  hub.setListen(true, 1);
  for (int i = 0; i < 4; ++i) {
    char id[48] = {};
    std::snprintf(id, sizeof(id), "shellyplussmoke-%012d", i);
    TEST_ASSERT_TRUE(hub.authorize(id, "x"));
  }
  TEST_ASSERT_FALSE(hub.authorize("shellyplussmoke-999999999999", "x"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_smoke_status_fields);
  RUN_TEST(test_reject_plug_id);
  RUN_TEST(test_authorize_and_ingest);
  RUN_TEST(test_stale_still_online_within_window);
  RUN_TEST(test_max_smoke_slots);
  return UNITY_END();
}
