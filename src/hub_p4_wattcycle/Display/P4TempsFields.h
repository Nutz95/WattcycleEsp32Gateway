#pragma once

namespace wattcycle::display {

constexpr int kP4ProbeSlots = 4;

/// Temperatures in whole degrees.
struct P4TempsFields {
  bool valid = false;
  int mosC = 0;
  int pcbC = 0;
  int probeCount = 0;
  int probeC[kP4ProbeSlots] = {};
};

}  // namespace wattcycle::display
