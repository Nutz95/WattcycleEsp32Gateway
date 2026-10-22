#pragma once

#include "Auth/AuthTypes.h"

#include "lvgl.h"

namespace wattcycle::panel {

/// Full-width tap target shown only while a web account needs a local confirm.
class P4AuthTap {
 public:
  /// Create the hidden bar on `parent`.
  void mount(lv_obj_t* parent);
  /// Show the bar that matches `kind`, or hide it.
  void apply(auth::AuthPromptKind kind);
  /// Called when the bar is tapped.
  void setHandler(void (*handler)(void* user), void* user);

 private:
  static void onClicked(lv_event_t* event);

  lv_obj_t* bar_ = nullptr;
  lv_obj_t* label_ = nullptr;
  auth::AuthPromptKind shown_ = auth::AuthPromptKind::None;
  void (*handler_)(void* user) = nullptr;
  void* user_ = nullptr;
};

}  // namespace wattcycle::panel
