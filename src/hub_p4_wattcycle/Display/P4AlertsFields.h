#pragma once

namespace wattcycle::display {

/// Alert page. `note` is a short line, empty when there is nothing to say.
struct P4AlertsFields {
  bool protection = false;
  bool fault = false;
  char note[48] = {};
};

}  // namespace wattcycle::display
