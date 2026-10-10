#pragma once

#include "Auth/AuthTypes.h"

#include <M5Unified.h>

namespace wattcycle::telemetry {
class ITelemetryStore;
}

namespace wattcycle::display {

void drawM5OverviewPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store);
void drawM5EcoflowPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store);
void drawM5PackPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store);
void drawM5SolarPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store);
void drawM5GatewayPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store);
void drawM5EspPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store);
void drawM5TempsPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store);
void drawM5ButtonFooter(M5Canvas& canvas, uint8_t pageIndex, const char* centerLabel);
void drawM5AuthPromptPage(M5Canvas& canvas, const auth::AuthPrompt& prompt);

}  // namespace wattcycle::display
