/// Pocket/LilyGO dongle TFT smoke — PSRAM double-buffer + APP_CPU UI task.
#include "boards/common/AppCoreDisplayUiTask.h"
#include "boards/common/IUiFrameSource.h"
#include "boards/dongle_s3/ActiveBoardPins.h"
#include "boards/dongle_s3/DonglePsramDoubleBuffer.h"
#include "boards/dongle_s3/Rgb565Color.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

namespace {

using boards::dongle_s3::DonglePsramDoubleBuffer;
using boards::dongle_s3::Rgb565Color;
using boards::dongle_s3::kBoardName;
using boards::dongle_s3::kDisplayHeight;
using boards::dongle_s3::kDisplayWidth;
using boards::dongle_s3::kTftPortraitHeight;
using boards::dongle_s3::kTftPortraitWidth;
using boards::ui::AppCoreDisplayUiTask;
using boards::ui::IUiFrameSource;

struct ColorStep {
  uint16_t color;
  const char* name;
};

constexpr ColorStep kSteps[] = {
    {Rgb565Color::fromRgb(255, 0, 0), "RED"},
    {Rgb565Color::fromRgb(0, 255, 0), "GREEN"},
    {Rgb565Color::fromRgb(0, 0, 255), "BLUE"},
    {Rgb565Color::fromRgb(255, 220, 0), "YELLOW"},
};

TFT_eSPI tft = TFT_eSPI(kTftPortraitWidth, kTftPortraitHeight);
DonglePsramDoubleBuffer frame(tft);

class SmokeUiSource : public IUiFrameSource {
 public:
  bool isDisplayAsleep() const override {
    return false;
  }

  void refreshIfDirty() override {
    const ColorStep& step = kSteps[stepIndex_];
    TFT_eSprite& s = frame.drawTarget();
    s.fillSprite(step.color);
    s.setTextDatum(MC_DATUM);
    s.setTextColor(Rgb565Color::fromRgb(255, 255, 255), step.color);
    s.drawString(step.name, kDisplayWidth / 2, kDisplayHeight / 2, 2);
    frame.present();
    Serial.printf("[smoke-ui] core=%d %s\n", xPortGetCoreID(), step.name);
    stepIndex_ = (stepIndex_ + 1) % (sizeof(kSteps) / sizeof(kSteps[0]));
  }

 private:
  size_t stepIndex_ = 0;
};

SmokeUiSource smokeSource;
AppCoreDisplayUiTask displayUi(smokeSource);

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println();
  Serial.printf("=== %s TFT smoke (PSRAM x2 + APP_CPU) ===\n", kBoardName);
  Serial.printf("heap=%u psram=%u setup_core=%d\n", ESP.getFreeHeap(), ESP.getFreePsram(),
                xPortGetCoreID());

  Serial.println(F("tft.begin..."));
  tft.begin();
  Serial.println(F("tft.begin ok"));
  tft.setRotation(1);

  if (!frame.begin(kDisplayWidth, kDisplayHeight)) {
    Serial.println(F("FATAL: PSRAM double buffer alloc failed"));
    return;
  }

  // One synchronous frame before the UI task takes over.
  smokeSource.refreshIfDirty();

  boards::ui::AppCoreDisplayUiTaskConfig displayUiConfig;
  displayUiConfig.taskName = "smokeUi";
  displayUiConfig.stackWords = 3072;
  displayUiConfig.priority = 1;
  displayUiConfig.refreshMs = 700;
  displayUiConfig.coreId = AppCoreDisplayUiTask::kAppCoreId;
  if (!displayUi.start(displayUiConfig)) {
    Serial.println(F("FATAL: display UI task failed"));
  }
  Serial.println(F("setup done — color cycle on APP_CPU"));
}

void loop() {
  delay(1000);
}
