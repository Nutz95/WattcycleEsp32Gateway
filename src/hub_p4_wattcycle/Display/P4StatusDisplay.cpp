#include "Display/P4StatusDisplay.h"

#include "Display/P4FlowBuilder.h"
#include "Display/P4PageBuilder.h"

#include <cstdio>

namespace wattcycle::display {
namespace {

HubPage stepPage(HubPage page, int delta) {
  const int count = static_cast<int>(kHubPageCount);
  int next = static_cast<int>(page) + delta;
  if (next < 0) {
    next = count - 1;
  }
  if (next >= count) {
    next = 0;
  }
  return static_cast<HubPage>(next);
}

}  // namespace

void P4StatusDisplay::begin() {
  page_ = HubPage::Overview;
  backlightOn_ = true;
  showingPrompt_ = false;
  last_ = {};
  lastFlow_ = {};
}

void P4StatusDisplay::render(const telemetry::ITelemetryStore& store) {
  showingPrompt_ = false;
  if (page_ == HubPage::Overview) {
    lastFlow_ = buildP4Overview(store);
    last_ = {};
    last_.page = HubPage::Overview;
    return;
  }
  lastFlow_ = {};
  last_ = buildP4Page(page_, store);
}

void P4StatusDisplay::renderAuthPrompt(const auth::AuthPrompt& prompt) {
  showingPrompt_ = true;
  lastFlow_ = {};
  last_ = {};
  last_.page = page_;
  const bool reset = prompt.kind == auth::AuthPromptKind::ConfirmReset;
  std::snprintf(last_.title, sizeof(last_.title), "%s", reset ? "Reset" : "Setup");
  std::snprintf(last_.line1, sizeof(last_.line1), "%s", prompt.username);
  std::snprintf(last_.line2, sizeof(last_.line2), "%s", reset ? "confirm reset" : "confirm setup");
}

void P4StatusDisplay::nextPage() {
  page_ = stepPage(page_, 1);
}

void P4StatusDisplay::previousPage() {
  page_ = stepPage(page_, -1);
}

uint8_t P4StatusDisplay::pageIndex() const {
  return static_cast<uint8_t>(page_);
}

void P4StatusDisplay::setBacklight(bool on) {
  backlightOn_ = on;
}

bool P4StatusDisplay::isBacklightOn() const {
  return backlightOn_;
}

const P4PageModel& P4StatusDisplay::lastModel() const {
  return last_;
}

const P4OverviewFlow& P4StatusDisplay::lastFlow() const {
  return lastFlow_;
}

bool P4StatusDisplay::showingPrompt() const {
  return showingPrompt_;
}

}  // namespace wattcycle::display
