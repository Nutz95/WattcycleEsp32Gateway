#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include <cstring>

#include "Telemetry/BinaryTelemetryCodec.h"
#include "Telemetry/InMemoryTelemetryStore.h"

using wattcycle::telemetry::BinaryTelemetryCodec;
using wattcycle::telemetry::InMemoryTelemetryStore;

void test_encode_magic_and_soc() {
  InMemoryTelemetryStore store;
  wattcycle::bms::BatteryTelemetry battery;
  battery.valid = true;
  battery.stateOfChargePercent = 88;
  battery.moduleVoltage = 51.2f;
  battery.currentAmps = -5.5f;
  battery.cellCount = 1;
  battery.cellVoltages[0] = 3.2f;
  battery.temperatureCount = 2;
  store.updateBattery(battery);
  store.setWifiState(true, "10.0.0.8");
  store.setBleState(true, "AA:BB:CC:DD:EE:FF", "");

  uint8_t buffer[512];
  const size_t written = BinaryTelemetryCodec::encode(store, buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(written > 32);
  TEST_ASSERT_EQUAL_HEX8(0x57, buffer[0]);  // W
  TEST_ASSERT_EQUAL_HEX8(0x54, buffer[1]);  // T
  TEST_ASSERT_EQUAL_HEX8(0x47, buffer[2]);  // G
  TEST_ASSERT_EQUAL_HEX8(0x4D, buffer[3]);  // M
  TEST_ASSERT_EQUAL_UINT8(BinaryTelemetryCodec::kVersion, buffer[4]);
  TEST_ASSERT_EQUAL_UINT8(88, buffer[6]);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_encode_magic_and_soc);
  return UNITY_END();
}
