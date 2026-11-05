#pragma once

#include "Display/IHubPageNavigator.h"
#include "Display/P4SmokeFields.h"

#include "lvgl.h"

namespace wattcycle::display {

/// Compact overview Smoke node with status LED; tap opens the Smoke page.
class P4SmokeOverviewCard {
 public:
  /// Build the card on the overview flow root.
  void mount(lv_obj_t* parent, IHubPageNavigator* navigator);
  /// Refresh LED color and status text from authorized detectors.
  void apply(const P4SmokeFields& smoke);

 private:
  lv_obj_t* card_ = nullptr;
  lv_obj_t* led_ = nullptr;
  lv_obj_t* status_ = nullptr;
};

}  // namespace wattcycle::display
