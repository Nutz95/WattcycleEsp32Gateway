#include "Display/P4DetailPages.h"

namespace wattcycle::display {

void applyEnergyPages(P4DetailWidgets& widgets, const P4PanelBoard& board);
void applySystemPages(P4DetailWidgets& widgets, const P4PanelBoard& board);

namespace {

void reveal(lv_obj_t* root, bool visible) {
  if (root == nullptr) {
    return;
  }
  if (visible) {
    lv_obj_clear_flag(root, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
  }
}

}  // namespace

void P4DetailPages::apply(const P4PanelBoard& board) {
  applyEnergyPages(widgets_, board);
  applySystemPages(widgets_, board);
}

void P4DetailPages::setMeterCommandHandler(void (*handler)(uint8_t command, void* user), void* user) {
  meterCommand_ = handler;
  meterUser_ = user;
}

void P4DetailPages::dispatchMeterCommand(uint8_t command) {
  if (meterCommand_ != nullptr) {
    meterCommand_(command, meterUser_);
  }
}

void P4DetailPages::show(HubPage page) {
  reveal(widgets_.wattcycle.root, page == HubPage::Wattcycle);
  reveal(widgets_.solar.root, page == HubPage::Solar);
  reveal(widgets_.eco.root, page == HubPage::EcoFlow);
  reveal(widgets_.cells.root, page == HubPage::Cells);
  reveal(widgets_.temps.root, page == HubPage::Temps);
  reveal(widgets_.alerts.root, page == HubPage::Alerts);
  reveal(widgets_.gateway.root, page == HubPage::Gateway);
  reveal(widgets_.esp.root, page == HubPage::Esp);
  reveal(widgets_.link.root, page == HubPage::Link);
  // Camera page is owned by P4CameraPage in the panel composition root.
}

}  // namespace wattcycle::display
