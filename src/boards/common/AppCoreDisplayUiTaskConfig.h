#pragma once

#include <freertos/FreeRTOS.h>

#include <cstdint>

namespace boards::ui {

/// Start parameters for AppCoreDisplayUiTask (APP_CPU UI pump).
struct AppCoreDisplayUiTaskConfig {
  const char* taskName = "displayUi";
  uint32_t stackWords = 3072;
  UBaseType_t priority = 1;
  uint32_t refreshMs = 500;
  /// Prefer AppCoreDisplayUiTask::kAppCoreId (1).
  int coreId = 1;
};

}  // namespace boards::ui
