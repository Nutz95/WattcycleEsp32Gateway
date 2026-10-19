#include "boards/common/AppCoreDisplayUiTask.h"

#include <Arduino.h>

namespace boards::ui {

AppCoreDisplayUiTask::AppCoreDisplayUiTask(IUiFrameSource& source) : source_(source) {}

bool AppCoreDisplayUiTask::start(const AppCoreDisplayUiTaskConfig& config) {
  if (handle_ != nullptr) {
    return true;
  }
  if (config.stackWords < 1024 || config.refreshMs == 0) {
    Serial.println(F("[app-ui] invalid StartConfig"));
    return false;
  }

  refreshMs_ = config.refreshMs;
  const BaseType_t ok = xTaskCreatePinnedToCore(
      &AppCoreDisplayUiTask::taskTrampoline, config.taskName, config.stackWords, this,
      config.priority, &handle_, config.coreId);
  if (ok != pdPASS || handle_ == nullptr) {
    handle_ = nullptr;
    Serial.println(F("[app-ui] xTaskCreatePinnedToCore failed"));
    return false;
  }

  Serial.printf("[app-ui] started '%s' core=%d stack=%u refresh=%ums\n", config.taskName,
                config.coreId, static_cast<unsigned>(config.stackWords),
                static_cast<unsigned>(refreshMs_));
  return true;
}

void AppCoreDisplayUiTask::notify() {
  if (handle_ != nullptr) {
    xTaskNotifyGive(handle_);
  }
}

void AppCoreDisplayUiTask::suspend() {
  if (handle_ != nullptr) {
    vTaskSuspend(handle_);
  }
}

void AppCoreDisplayUiTask::resume() {
  if (handle_ != nullptr) {
    vTaskResume(handle_);
    xTaskNotifyGive(handle_);
  }
}

bool AppCoreDisplayUiTask::running() const {
  return handle_ != nullptr;
}

TaskHandle_t AppCoreDisplayUiTask::handle() const {
  return handle_;
}

void AppCoreDisplayUiTask::taskTrampoline(void* context) {
  static_cast<AppCoreDisplayUiTask*>(context)->taskLoop();
}

void AppCoreDisplayUiTask::taskLoop() {
  for (;;) {
    if (source_.isDisplayAsleep()) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      continue;
    }

    source_.refreshIfDirty();
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(refreshMs_));
  }
}

}  // namespace boards::ui
