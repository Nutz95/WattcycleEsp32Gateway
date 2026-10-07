#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include <cstring>

#include "Telemetry/InMemoryTelemetryStore.h"
#include "Telemetry/TelemetryJsonSerializer.h"

using wattcycle::telemetry::InMemoryTelemetryStore;
using wattcycle::telemetry::TelemetryJsonSerializer;

void test_serialize_includes_core_fields() {
  InMemoryTelemetryStore store;
  wattcycle::bms::BatteryTelemetry battery;
  battery.valid = true;
  battery.stateOfChargePercent = 77;
  battery.moduleVoltage = 51.2f;
  battery.currentAmps = -12.5f;
  battery.powerWatts = -640.0f;
  battery.cellCount = 1;
  battery.cellVoltages[0] = 3.2f;
  store.updateBattery(battery);
  store.setWifiState(true, "192.168.1.50");

  char json[1024];
  const size_t written = TelemetryJsonSerializer::serialize(store, json, sizeof(json));
  TEST_ASSERT_TRUE(written > 0);
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"soc\":77"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "192.168.1.50"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"wifi\":true"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_serialize_includes_core_fields);
  return UNITY_END();
}
