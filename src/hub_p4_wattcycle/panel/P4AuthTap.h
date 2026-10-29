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
  /// Called when the confirm bar is tapped.
  void setHandler(void (*handler)(void* user), void* user);
  /// Called when Cancel is tapped.
  void setCancelHandler(void (*handler)(void* user), void* user);

 private:
  static void onClicked(lv_event_t* event);
  static void onCancelClicked(lv_event_t* event);

  lv_obj_t* bar_ = nullptr;
  lv_obj_t* label_ = nullptr;
  lv_obj_t* cancel_ = nullptr;
  auth::AuthPromptKind shown_ = auth::AuthPromptKind::None;
  void (*handler_)(void* user) = nullptr;
  void* user_ = nullptr;
  void (*cancelHandler_)(void* user) = nullptr;
  void* cancelUser_ = nullptr;
};

}  // namespace wattcycle::panel
