#include "P4CameraPage.h"

#include "P4LiveView.h"

#include "Display/P4DetailRow.h"
#include "Display/P4OverviewStyle.h"

#include <cstdio>
#include <cstring>

#include <esp_heap_caps.h>

namespace wattcycle::panel {
namespace {

constexpr int kPreviewW = 320;
constexpr int kPreviewH = 180;

void onMicSlider(lv_event_t* event) {
  auto* mic = static_cast<audio::IMicGain*>(lv_event_get_user_data(event));
  if (mic == nullptr) {
    return;
  }
  mic->setPercent(lv_slider_get_value(lv_event_get_target(event)));
}

}  // namespace

void P4CameraPage::mount(lv_obj_t* parent, P4LiveView& live, audio::IMicGain& mic) {
  live_ = &live;
  mic_ = &mic;
  root_ = wattcycle::display::makeDetailRoot(parent);
  lv_obj_t* title = lv_label_create(root_);
  lv_label_set_text(title, "Camera");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(wattcycle::display::kInk), 0);
  lv_obj_set_pos(title, 16, 12);

  status_ = lv_label_create(root_);
  lv_label_set_text(status_, "Idle");
  lv_obj_set_width(status_, 448);
  lv_obj_set_style_text_font(status_, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(status_, lv_color_hex(wattcycle::display::kMuted), 0);
  lv_obj_set_pos(status_, 16, 52);

  canvasBuf_ = static_cast<lv_color_t*>(
      heap_caps_malloc(sizeof(lv_color_t) * kPreviewW * kPreviewH, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  canvas_ = lv_canvas_create(root_);
  lv_obj_set_pos(canvas_, 80, 100);
  lv_obj_set_size(canvas_, kPreviewW, kPreviewH);
  if (canvasBuf_ != nullptr) {
    lv_canvas_set_buffer(canvas_, canvasBuf_, kPreviewW, kPreviewH, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas_, lv_color_hex(0x000000), LV_OPA_COVER);
  }

  lv_obj_t* vuTrack = lv_obj_create(root_);
  lv_obj_set_pos(vuTrack, 16, 300);
  lv_obj_set_size(vuTrack, 448, 28);
  lv_obj_set_style_bg_color(vuTrack, lv_color_hex(wattcycle::display::kBg), 0);
  lv_obj_set_style_border_color(vuTrack, lv_color_hex(wattcycle::display::kIdleEdge), 0);
  lv_obj_set_style_border_width(vuTrack, 2, 0);
  lv_obj_set_style_radius(vuTrack, 6, 0);
  lv_obj_set_style_pad_all(vuTrack, 0, 0);
  vuFill_ = lv_obj_create(vuTrack);
  lv_obj_set_pos(vuFill_, 2, 2);
  lv_obj_set_size(vuFill_, 4, 24);
  lv_obj_set_style_bg_color(vuFill_, lv_color_hex(wattcycle::display::kAccent), 0);
  lv_obj_set_style_border_width(vuFill_, 0, 0);
  lv_obj_set_style_radius(vuFill_, 4, 0);

  lv_obj_t* volumeName = lv_label_create(root_);
  lv_label_set_text(volumeName, "Volume");
  lv_obj_set_style_text_font(volumeName, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(volumeName, lv_color_hex(wattcycle::display::kMuted), 0);
  lv_obj_set_pos(volumeName, 16, 348);
  lv_obj_t* volume = lv_slider_create(root_);
  lv_obj_set_pos(volume, 110, 348);
  lv_obj_set_width(volume, 340);
  lv_slider_set_range(volume, 0, 100);
  lv_slider_set_value(volume, mic_->percent(), LV_ANIM_OFF);
  lv_obj_add_event_cb(volume, onMicSlider, LV_EVENT_VALUE_CHANGED, mic_);

  lv_obj_t* hint = lv_label_create(root_);
  lv_label_set_text(hint, "Web stream is blocked while this page is open.\nLeave the page to free the camera.");
  lv_obj_set_width(hint, 448);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex(wattcycle::display::kMuted), 0);
  lv_obj_set_pos(hint, 16, 400);
}

void P4CameraPage::show(bool visible) {
  if (root_ == nullptr || live_ == nullptr) {
    return;
  }
  visible_ = visible;
  live_->requestPanel(visible);
  if (visible) {
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(status_, "Opening camera…");
  } else {
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(status_, "Idle");
  }
}

void P4CameraPage::tick() {
  if (!visible_ || live_ == nullptr || canvasBuf_ == nullptr) {
    return;
  }
  if (!live_->panelClaimed()) {
    lv_label_set_text(status_, live_->wantsBlank() ? "Busy: web is streaming. Close the Camera tab first."
                                                   : "Opening camera…");
    return;
  }
  int peak = 0;
  auto* pixels = reinterpret_cast<uint16_t*>(canvasBuf_);
  if (!live_->pullPanelFrame(pixels, kPreviewW, kPreviewH, peak)) {
    const char* fail = live_->cameraFail();
    if (fail != nullptr && fail[0] != '\0') {
      char line[96] = {};
      std::snprintf(line, sizeof(line), "Camera: %s", fail);
      lv_label_set_text(status_, line);
    } else {
      lv_label_set_text(status_, "Waiting for first frame…");
    }
    return;
  }
  lv_label_set_text(status_, "Live preview");
  lv_obj_invalidate(canvas_);
  if (vuFill_ != nullptr) {
    const int width = 4 + ((440 * peak) / 100);
    lv_obj_set_width(vuFill_, width < 4 ? 4 : width);
  }
}

}  // namespace wattcycle::panel
