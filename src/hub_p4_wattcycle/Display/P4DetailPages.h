#pragma once

#include "Display/HubPage.h"
#include "Display/P4DetailWidgets.h"
#include "Display/P4PanelBoard.h"

namespace wattcycle::display {

/// Detail screens. They read `P4PanelBoard` and do not own the radios.
class P4DetailPages {
 public:
  /// Build every page under `parent`. They stay hidden until `show`.
  void mount(lv_obj_t* parent);
  /// Paint every page from the current document.
  void apply(const P4PanelBoard& board);
  /// Show one page. Overview hides them all.
  void show(HubPage page);

 private:
  P4DetailWidgets widgets_;
};

}  // namespace wattcycle::display
