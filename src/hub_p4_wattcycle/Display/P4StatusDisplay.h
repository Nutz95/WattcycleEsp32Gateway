#pragma once

#include "Display/IStatusDisplay.h"
#include "Display/P4OverviewFlow.h"
#include "Display/P4PageModel.h"

namespace wattcycle::display {

/// P4 page shell. Overview is `lastFlow()`. Other pages are `lastModel()`.
class P4StatusDisplay : public IStatusDisplay {
 public:
  /// Prepare the shell on the overview page with the backlight on.
  void begin() override;
  /// Rebuild the overview flow or the detail text for the active page.
  void render(const telemetry::ITelemetryStore& store) override;
  /// Show a confirm-setup or confirm-reset prompt instead of a page.
  void renderAuthPrompt(const auth::AuthPrompt& prompt) override;
  /// Advance one page, wrapping after ESP.
  void nextPage() override;
  /// Go back one page, wrapping from overview to ESP.
  void previousPage() override;
  /// Active page index, matching `HubPage`.
  uint8_t pageIndex() const override;
  /// Remember the backlight state for the future panel driver.
  void setBacklight(bool on) override;
  /// Last backlight value passed to `setBacklight`.
  bool isBacklightOn() const override;
  /// Text for a detail page or the auth prompt. Empty lines on the overview.
  const P4PageModel& lastModel() const;
  /// Overview diagram from the last render. Ignored while a prompt is showing.
  const P4OverviewFlow& lastFlow() const;
  /// True after `renderAuthPrompt` until the next `render`.
  bool showingPrompt() const;

 private:
  HubPage page_ = HubPage::Overview;
  bool backlightOn_ = true;
  bool showingPrompt_ = false;
  P4PageModel last_{};
  P4OverviewFlow lastFlow_{};
};

}  // namespace wattcycle::display
