#pragma once

#include "Bms/Models/BatteryTelemetry.h"
#include "Storage/IDailyHistoryStore.h"
#include "Telemetry/EcoFlowBridgeTelemetry.h"
#include "Telemetry/SolarBridgeTelemetry.h"

namespace wattcycle::storage {

/// Copy the latest pack, meter, and EcoFlow readings into one day-file row.
void fillHistorySample(HistorySample& sample, const bms::BatteryTelemetry& battery,
                       const telemetry::SolarBridgeTelemetry& solar,
                       const telemetry::EcoFlowBridgeTelemetry& eco);

}  // namespace wattcycle::storage
