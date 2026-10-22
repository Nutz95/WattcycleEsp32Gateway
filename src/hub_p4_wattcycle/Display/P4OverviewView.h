#pragma once

#include "Display/P4OverviewFlow.h"

#include "lvgl.h"

namespace wattcycle::display {

/// LVGL overview for the 480×800 panel. Paints `P4OverviewFlow` and nothing else.
class P4OverviewView {
 public:
  /// Build the cards on `parent`. `footer` is a one-line note under the diagram.
  void mount(lv_obj_t* parent, const char* footer);
  /// Refresh labels and link colors from the latest flow snapshot.
  void apply(const P4OverviewFlow& flow);

 private:
  void mountEco(lv_obj_t* parent);
  void mountPorts(lv_obj_t* parent);
  void mountPack(lv_obj_t* parent);
  void mountSolar(lv_obj_t* parent);
  void applyEco(const P4OverviewFlow& flow);
  void applyPorts(const P4OverviewFlow& flow);
  void applyPack(const P4OverviewFlow& flow);
  void applySolar(const P4OverviewFlow& flow);

  lv_obj_t* ecoCard_ = nullptr;
  lv_obj_t* ecoSoc_ = nullptr;
  lv_obj_t* ecoBatt_ = nullptr;
  lv_obj_t* ecoBalance_ = nullptr;
  lv_obj_t* ecoSelf_ = nullptr;
  lv_obj_t* acIn_ = nullptr;
  lv_obj_t* acOut_ = nullptr;
  lv_obj_t* dcOut_ = nullptr;
  lv_obj_t* usbOut_ = nullptr;
  lv_obj_t* packCard_ = nullptr;
  lv_obj_t* packSoc_ = nullptr;
  lv_obj_t* packWatts_ = nullptr;
  lv_obj_t* solarCard_ = nullptr;
  lv_obj_t* solarWatts_ = nullptr;
};

}  // namespace wattcycle::display
