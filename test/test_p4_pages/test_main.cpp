#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Display/P4FlowBuilder.h"
#include "Display/P4PageBuilder.h"
#include "Display/P4StatusDisplay.h"
#include "Telemetry/InMemoryTelemetryStore.h"

void test_overview_flow_splits_self_from_battery() {
  wattcycle::telemetry::InMemoryTelemetryStore store;
  wattcycle::bms::BatteryTelemetry battery;
  battery.valid = true;
  battery.stateOfChargePercent = 80;
  battery.powerWatts = -71.0f;
  store.updateBattery(battery);
  store.setBmsLink(true, true);

  wattcycle::telemetry::EcoFlowBridgeTelemetry eco;
  eco.meterValid = true;
  eco.linkFresh = true;
  eco.bleConnected = true;
  eco.socPercent = 18;
  eco.bmsPowerW = -71;
  eco.acOutputW = 93;
  eco.acOutputOn = true;
  store.updateEcoFlow(eco);

  const auto flow = wattcycle::display::buildP4Overview(store);
  TEST_ASSERT_EQUAL(static_cast<int>(wattcycle::display::P4Link::Up), static_cast<int>(flow.packLink));
  TEST_ASSERT_EQUAL(static_cast<int>(wattcycle::display::P4Link::Up), static_cast<int>(flow.ecoLink));
  TEST_ASSERT_TRUE(flow.packEdge.active);
  TEST_ASSERT_TRUE(flow.packEdge.reverse);
  TEST_ASSERT_EQUAL_INT(-71, flow.packWatts);
  TEST_ASSERT_TRUE(flow.showEcoBattery);
  TEST_ASSERT_EQUAL_INT(-71, flow.ecoBatteryW);
  TEST_ASSERT_TRUE(flow.showBalance);
  TEST_ASSERT_EQUAL_INT(-22, flow.balanceW);
  TEST_ASSERT_TRUE(flow.showSelf);
  TEST_ASSERT_EQUAL_INT(49, flow.selfW);
  TEST_ASSERT_TRUE(flow.acOutOn);
  TEST_ASSERT_EQUAL_INT(93, flow.acOutW);

  wattcycle::display::P4StatusDisplay display;
  display.begin();
  display.render(store);
  TEST_ASSERT_EQUAL_INT(49, display.lastFlow().selfW);
  TEST_ASSERT_EQUAL_STRING("", display.lastModel().line1);
  TEST_ASSERT_FALSE(display.showingPrompt());
}

void test_page_wraps_and_keeps_backlight() {
  wattcycle::telemetry::InMemoryTelemetryStore store;
  wattcycle::display::P4StatusDisplay display;
  display.begin();
  TEST_ASSERT_EQUAL_UINT8(0, display.pageIndex());
  TEST_ASSERT_TRUE(display.isBacklightOn());

  display.previousPage();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(wattcycle::display::HubPage::Esp), display.pageIndex());
  display.nextPage();
  TEST_ASSERT_EQUAL_UINT8(0, display.pageIndex());

  display.setBacklight(false);
  display.render(store);
  TEST_ASSERT_FALSE(display.isBacklightOn());
  TEST_ASSERT_EQUAL_STRING("", display.lastModel().line1);
  TEST_ASSERT_FALSE(display.lastFlow().showSelf);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_overview_flow_splits_self_from_battery);
  RUN_TEST(test_page_wraps_and_keeps_backlight);
  return UNITY_END();
}
