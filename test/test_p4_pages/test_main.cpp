#include <unity.h>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

#include "Display/P4DemoBoard.h"
#include "Display/P4FlowBuilder.h"
#include "Display/P4PageBuilder.h"
#include "Display/P4SolarRoute.h"
#include "Display/P4StatusDisplay.h"
#include "Telemetry/InMemoryTelemetryStore.h"
#include "Telemetry/SolarBridgeTelemetry.h"

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
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(wattcycle::display::HubPage::Plugs), display.pageIndex());
  display.nextPage();
  TEST_ASSERT_EQUAL_UINT8(0, display.pageIndex());

  display.setBacklight(false);
  display.render(store);
  TEST_ASSERT_FALSE(display.isBacklightOn());
  TEST_ASSERT_EQUAL_STRING("", display.lastModel().line1);
  TEST_ASSERT_FALSE(display.lastFlow().showSelf);
}

void test_demo_board_matches_locked_fixture() {
  const auto board = wattcycle::display::demoBoard();
  TEST_ASSERT_EQUAL_INT(-71, board.overview.packWatts);
  TEST_ASSERT_EQUAL_INT(-71, board.wattcycle.watts);
  TEST_ASSERT_EQUAL_INT(-71, board.eco.batteryW);
  TEST_ASSERT_EQUAL_INT(93, board.overview.acOutW);
  TEST_ASSERT_EQUAL_INT(93, board.eco.acW);
  TEST_ASSERT_EQUAL_INT(-22, board.overview.balanceW);
  TEST_ASSERT_EQUAL_INT(49, board.overview.selfW);
  TEST_ASSERT_EQUAL_INT(80, board.wattcycle.soc);
  TEST_ASSERT_EQUAL_INT(13200, board.wattcycle.voltsMv);
  TEST_ASSERT_EQUAL_INT(-5380, board.wattcycle.ampsMa);
  TEST_ASSERT_FALSE(board.solar.linkUp);
  TEST_ASSERT_FALSE(board.overview.solarActive);
  TEST_ASSERT_FALSE(board.gateway.wifiUp);
  TEST_ASSERT_TRUE(board.gateway.bmsUp);
  TEST_ASSERT_TRUE(board.gateway.ecoUp);
  TEST_ASSERT_EQUAL_STRING("", board.gateway.hubMac);
  TEST_ASSERT_EQUAL_STRING("", board.gateway.bmsMac);
  TEST_ASSERT_EQUAL_STRING("", board.esp.deviceMac);
  TEST_ASSERT_EQUAL_INT(4, board.cells.count);
  TEST_ASSERT_TRUE(board.cells.balancing[1]);
  TEST_ASSERT_EQUAL_INT(30, board.cells.voltsMv[1] - board.cells.voltsMv[0]);
  TEST_ASSERT_EQUAL_INT(2, board.temps.probeCount);
  TEST_ASSERT_EQUAL_STRING("none", board.alerts.note);
  const auto route = wattcycle::display::solarRoute(board.overview);
  TEST_ASSERT_FALSE(route.toPack);
  TEST_ASSERT_FALSE(route.toEco);
}

void test_solar_route_follows_pack_direction() {
  wattcycle::telemetry::InMemoryTelemetryStore store;
  wattcycle::bms::BatteryTelemetry battery;
  battery.valid = true;
  battery.powerWatts = -71.0f;
  store.updateBattery(battery);
  store.setBmsLink(true, true);

  wattcycle::telemetry::SolarBridgeTelemetry solar;
  solar.linkFresh = true;
  solar.sppConnected = true;
  solar.meterValid = true;
  solar.powerW = 40.0f;
  store.updateSolar(solar);

  auto flow = wattcycle::display::buildP4Overview(store);
  auto route = wattcycle::display::solarRoute(flow);
  TEST_ASSERT_TRUE(flow.solarActive);
  TEST_ASSERT_TRUE(flow.packEdge.reverse);
  TEST_ASSERT_FALSE(route.toPack);
  TEST_ASSERT_TRUE(route.toEco);

  battery.powerWatts = 40.0f;
  store.updateBattery(battery);
  flow = wattcycle::display::buildP4Overview(store);
  route = wattcycle::display::solarRoute(flow);
  TEST_ASSERT_FALSE(flow.packEdge.reverse);
  TEST_ASSERT_TRUE(route.toPack);
  TEST_ASSERT_FALSE(route.toEco);

  solar.powerW = 0.0f;
  store.updateSolar(solar);
  flow = wattcycle::display::buildP4Overview(store);
  route = wattcycle::display::solarRoute(flow);
  TEST_ASSERT_FALSE(flow.solarActive);
  TEST_ASSERT_FALSE(route.toPack);
  TEST_ASSERT_FALSE(route.toEco);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_overview_flow_splits_self_from_battery);
  RUN_TEST(test_page_wraps_and_keeps_backlight);
  RUN_TEST(test_demo_board_matches_locked_fixture);
  RUN_TEST(test_solar_route_follows_pack_direction);
  return UNITY_END();
}
