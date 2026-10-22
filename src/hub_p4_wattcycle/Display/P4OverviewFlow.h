#pragma once

#include "Display/P4FlowEdge.h"
#include "Display/P4Link.h"

#include <cstdint>

namespace wattcycle::display {

/// Overview diagram for the 480×800 panel. Values follow the web flow rules.
/// EcoFlow battery watts keep the station sign: negative discharges, positive charges.
struct P4OverviewFlow {
  P4Link packLink = P4Link::Down;
  P4Link ecoLink = P4Link::Down;
  P4Link solarLink = P4Link::Down;
  bool packValid = false;
  uint16_t packSoc = 0;
  /// Signed Wattcycle watts. Negative sends power toward the EcoFlow.
  int packWatts = 0;
  P4FlowEdge packEdge{};
  bool ecoValid = false;
  uint8_t ecoSoc = 0;
  bool showEcoBattery = false;
  int ecoBatteryW = 0;
  /// Port gap. Negative means outputs exceed inputs ("out−in").
  bool showBalance = false;
  int balanceW = 0;
  /// Station self-consumption, shown only as positive watts.
  bool showSelf = false;
  int selfW = 0;
  bool acInActive = false;
  int acInW = 0;
  bool acOutOn = false;
  int acOutW = 0;
  bool dcOutOn = false;
  int dcOutW = 0;
  bool usbOutOn = false;
  int usbOutW = 0;
  bool solarActive = false;
  int solarW = 0;
};

}  // namespace wattcycle::display
