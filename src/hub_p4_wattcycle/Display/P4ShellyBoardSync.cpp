#include "Display/P4ShellyBoardSync.h"

#include "Util/SafeCopy.h"

namespace wattcycle::display {

void fillPlugsFields(P4PlugsFields& fields, const shelly::IShellyPlugHub& hub, uint32_t nowMs) {
  fields = {};
  shelly::ShellyPlugReading readings[shelly::kMaxShellyPlugs] = {};
  const size_t count = hub.snapshotAuthorized(readings, shelly::kMaxShellyPlugs, nowMs);
  float total = 0;
  for (size_t i = 0; i < count; ++i) {
    util::copyCString(fields.names[i], sizeof(fields.names[i]), readings[i].name);
    fields.watts[i] = readings[i].watts;
    fields.whToday[i] = readings[i].whToday;
    fields.online[i] = readings[i].online;
    fields.output[i] = readings[i].output;
    if (readings[i].online) {
      total += readings[i].watts;
    }
  }
  fields.count = static_cast<uint8_t>(count);
  fields.totalWatts = total;
}

void fillSmokeFields(P4SmokeFields& fields, const shelly::IShellySmokeHub& hub, uint32_t nowMs) {
  fields = {};
  shelly::ShellySmokeReading readings[shelly::kMaxShellySmoke] = {};
  const size_t count = hub.snapshotAuthorized(readings, shelly::kMaxShellySmoke, nowMs);
  for (size_t i = 0; i < count; ++i) {
    util::copyCString(fields.names[i], sizeof(fields.names[i]), readings[i].name);
    fields.alarm[i] = readings[i].alarm;
    fields.online[i] = readings[i].online;
    fields.batteryPercent[i] = readings[i].batteryPercent;
    if (readings[i].alarm) {
      fields.anyAlarm = true;
    }
  }
  fields.count = static_cast<uint8_t>(count);
}

}  // namespace wattcycle::display
