#include "Display/P4LiveBoard.h"

#include "Display/P4FlowBuilder.h"

#include <Arduino.h>

#include <cmath>
#include <cstdio>

namespace wattcycle::display {
namespace {

int roundToInt(float value) {
  return static_cast<int>(std::floor(value + 0.5f));
}

void fillPack(P4PanelBoard& board, const telemetry::ITelemetryStore& store) {
  const auto battery = store.battery();
  const auto warnings = store.warnings();
  const auto status = store.status();
  board.wattcycle.linkUp = status.telemetryFresh && status.bleConnected;
  board.wattcycle.secured = status.bmsEspNowEncrypted;
  board.wattcycle.valid = battery.valid;
  board.wattcycle.soc = battery.stateOfChargePercent;
  board.wattcycle.voltsMv = roundToInt(battery.moduleVoltage * 1000.0f);
  board.wattcycle.ampsMa = roundToInt(battery.currentAmps * 1000.0f);
  board.wattcycle.watts = roundToInt(battery.powerWatts);
  board.wattcycle.remainAhTimes10 = roundToInt(battery.remainingCapacityAh * 10.0f);
  board.wattcycle.totalAhTimes10 = roundToInt(battery.totalCapacityAh * 10.0f);
  board.wattcycle.cycles = battery.cycleNumber;
  board.wattcycle.soh = battery.stateOfHealthPercent;

  const int cells = battery.cellCount < kP4CellSlots ? battery.cellCount : kP4CellSlots;
  board.cells.count = cells;
  for (int i = 0; i < cells; ++i) {
    board.cells.voltsMv[i] = roundToInt(battery.cellVoltages[i] * 1000.0f);
    board.cells.balancing[i] = warnings.valid && warnings.cellBalancing[i];
  }
  board.temps.valid = battery.valid;
  board.temps.mosC = roundToInt(battery.mosTemperatureC);
  board.temps.pcbC = roundToInt(battery.pcbTemperatureC);
  const int probes = battery.temperatureCount < kP4ProbeSlots ? battery.temperatureCount : kP4ProbeSlots;
  board.temps.probeCount = probes;
  for (int i = 0; i < probes; ++i) {
    board.temps.probeC[i] = roundToInt(battery.cellTemperaturesC[i]);
  }
  board.alerts.protection = warnings.hasActiveProtection;
  board.alerts.fault = warnings.hasFault;
  const char* note = "none";
  if (warnings.hasFault) {
    note = "fault";
  } else if (warnings.hasActiveProtection) {
    note = "protection";
  }
  std::snprintf(board.alerts.note, sizeof(board.alerts.note), "%s", note);
}

void fillStations(P4PanelBoard& board, const telemetry::ITelemetryStore& store) {
  const auto solar = store.solar();
  const auto eco = store.ecoflow();
  board.solar.linkUp = solar.linkFresh && solar.sppConnected;
  board.solar.secured = solar.espNowEncrypted;
  board.solar.watts = roundToInt(solar.powerW);
  board.solar.voltsMv = roundToInt(solar.voltageV * 1000.0f);
  board.eco.linkUp = eco.linkFresh && eco.bleConnected;
  board.eco.secured = eco.espNowEncrypted;
  board.eco.valid = eco.meterValid;
  board.eco.soc = eco.socPercent;
  board.eco.batteryW = eco.bmsPowerW;
  board.eco.acOn = eco.acOutputOn;
  board.eco.acW = eco.acOutputW;
  board.eco.dcOn = eco.dcOutputOn;
  board.eco.dcW = eco.dcOutputW;
  board.eco.usbOn = eco.usbOutputOn;
  board.eco.usbW = eco.usbOutputW;
  board.gateway.bmsUp = board.wattcycle.linkUp;
  board.gateway.ecoUp = board.eco.linkUp;
  board.gateway.solarUp = board.solar.linkUp;
}

}  // namespace

void fillP4LiveBoard(P4PanelBoard& board, const telemetry::ITelemetryStore& store) {
  board.overview = buildP4Overview(store);
  fillPack(board, store);
  fillStations(board, store);
  board.esp.heapKb = static_cast<int>(ESP.getFreeHeap() / 1024u);
}

}  // namespace wattcycle::display
