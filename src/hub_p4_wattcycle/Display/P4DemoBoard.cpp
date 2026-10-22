#include "Display/P4DemoBoard.h"

#include <cstdio>

namespace wattcycle::display {

P4PanelBoard demoBoard() {
  P4PanelBoard board;
  board.overview.packLink = P4Link::Up;
  board.overview.ecoLink = P4Link::Up;
  board.overview.solarLink = P4Link::Down;
  board.overview.packValid = true;
  board.overview.packSoc = 80;
  board.overview.packWatts = -71;
  board.overview.packEdge.active = true;
  board.overview.packEdge.reverse = true;
  board.overview.ecoValid = true;
  board.overview.ecoSoc = 18;
  board.overview.showEcoBattery = true;
  board.overview.ecoBatteryW = -71;
  board.overview.showBalance = true;
  board.overview.balanceW = -22;
  board.overview.showSelf = true;
  board.overview.selfW = 49;
  board.overview.acOutOn = true;
  board.overview.acOutW = 93;

  board.wattcycle.linkUp = true;
  board.wattcycle.valid = true;
  board.wattcycle.soc = 80;
  board.wattcycle.voltsMv = 13200;
  board.wattcycle.ampsMa = -5380;
  board.wattcycle.watts = -71;
  board.wattcycle.remainAhTimes10 = 804;
  board.wattcycle.totalAhTimes10 = 1000;
  board.wattcycle.cycles = 120;
  board.wattcycle.soh = 98;

  board.solar.linkUp = false;
  board.eco.linkUp = true;
  board.eco.valid = true;
  board.eco.soc = 18;
  board.eco.batteryW = -71;
  board.eco.acOn = true;
  board.eco.acW = 93;

  board.cells.count = 4;
  board.cells.voltsMv[0] = 3280;
  board.cells.voltsMv[1] = 3310;
  board.cells.voltsMv[2] = 3294;
  board.cells.voltsMv[3] = 3302;
  board.cells.balancing[1] = true;

  board.temps.valid = true;
  board.temps.mosC = 32;
  board.temps.pcbC = 29;
  board.temps.probeCount = 2;
  board.temps.probeC[0] = 28;
  board.temps.probeC[1] = 30;

  std::snprintf(board.alerts.note, sizeof(board.alerts.note), "none");
  board.gateway.bmsUp = true;
  board.gateway.ecoUp = true;
  // MAC slots stay empty in the demo. Live fill uses STA + build-time peer env.
  copyMacText(board.gateway.hubMac, "");
  copyMacText(board.gateway.bmsMac, "");
  copyMacText(board.gateway.ecoMac, "");
  copyMacText(board.gateway.solarMac, "");
  copyMacText(board.esp.deviceMac, "");
  board.esp.heapKb = 180;
  board.esp.cpu0 = 14;
  board.esp.cpu1 = 9;
  return board;
}

}  // namespace wattcycle::display
