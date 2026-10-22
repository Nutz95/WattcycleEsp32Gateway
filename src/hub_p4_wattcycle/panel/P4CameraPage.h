#pragma once

#include "lvgl.h"

namespace wattcycle::panel {

class P4LiveView;

/// Local Camera debug page: preview canvas + mic vu-meter.
class P4CameraPage {
 public:
  void mount(lv_obj_t* parent, P4LiveView& live);
  void show(bool visible);
  void tick();

 private:
  P4LiveView* live_ = nullptr;
  lv_obj_t* root_ = nullptr;
  lv_obj_t* status_ = nullptr;
  lv_obj_t* canvas_ = nullptr;
  lv_obj_t* vuFill_ = nullptr;
  lv_color_t* canvasBuf_ = nullptr;
  bool visible_ = false;
};

}  // namespace wattcycle::panel
