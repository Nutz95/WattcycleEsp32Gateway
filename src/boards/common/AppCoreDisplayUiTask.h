#pragma once

#include "boards/common/AppCoreDisplayUiTaskConfig.h"
#include "boards/common/IUiFrameSource.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdint>

namespace boards::ui {

/// Pins TFT / heavy UI work to APP_CPU so BLE / Wi-Fi / ESP-NOW stay on PRO_CPU.
///
/// Typical wiring: radio tasks → kRadioCoreId (0); this task → kAppCoreId (1).
/// On Pocket-Dongle, pair with DonglePsramDoubleBuffer for tear-resistant presents.
class AppCoreDisplayUiTask {
 public:
  /// PRO_CPU — Wi-Fi / BT / ESP-NOW affinity.
  static constexpr int kRadioCoreId = 0;
  /// APP_CPU — sprite draw + SPI push.
  static constexpr int kAppCoreId = 1;

  /// Bind a frame source owned by the composition root.
  explicit AppCoreDisplayUiTask(IUiFrameSource& source);

  /// Create the pinned FreeRTOS task (idempotent).
  bool start(const AppCoreDisplayUiTaskConfig& config);

  /// Wake early after a button / telemetry change.
  void notify();

  /// Suspend the pump while the panel sleeps.
  void suspend();

  /// Resume after wakeDisplay().
  void resume();

  /// True after a successful start().
  bool running() const;

  /// Underlying FreeRTOS handle (may be nullptr).
  TaskHandle_t handle() const;

 private:
  static void taskTrampoline(void* context);
  void taskLoop();

  IUiFrameSource& source_;
  TaskHandle_t handle_ = nullptr;
  uint32_t refreshMs_ = 500;
};

}  // namespace boards::ui
