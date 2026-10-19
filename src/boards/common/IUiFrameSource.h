#pragma once

namespace boards::ui {

/// Frame source for AppCoreDisplayUiTask (drawn only on APP_CPU).
class IUiFrameSource {
 public:
  virtual ~IUiFrameSource() = default;

  /// True while the panel should not refresh (backlight sleep).
  virtual bool isDisplayAsleep() const = 0;

  /// Draw dirty content (e.g. PSRAM back-buffer + present) on APP_CPU.
  virtual void refreshIfDirty() = 0;
};

}  // namespace boards::ui
