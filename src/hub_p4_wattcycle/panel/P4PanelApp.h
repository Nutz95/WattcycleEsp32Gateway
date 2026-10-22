#pragma once

namespace wattcycle::panel {

/// Bring up the JC MIPI panel and paint the demo overview.
void setupPanel();
/// Pump LVGL. Call from the sketch loop.
void loopPanel();

}  // namespace wattcycle::panel
