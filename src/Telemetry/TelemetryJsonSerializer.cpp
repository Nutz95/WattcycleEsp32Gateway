#include "Telemetry/TelemetryJsonSerializer.h"

#include <ArduinoJson.h>

namespace wattcycle::telemetry {

size_t TelemetryJsonSerializer::serialize(const ITelemetryStore& store, char* buffer,
                                          size_t capacity) {
  if (buffer == nullptr || capacity < 64) {
    return 0;
  }

  const auto battery = store.battery();
  const auto product = store.product();
  const auto warnings = store.warnings();
  const auto gateway = store.status();

  JsonDocument document;
  document["valid"] = battery.valid;
  document["soc"] = battery.stateOfChargePercent;
  document["voltage"] = battery.moduleVoltage;
  document["current"] = battery.currentAmps;
  document["power"] = battery.powerWatts;
  document["remainingAh"] = battery.remainingCapacityAh;
  document["totalAh"] = battery.totalCapacityAh;
  document["cycles"] = battery.cycleNumber;
  document["mosTemp"] = battery.mosTemperatureC;
  document["pcbTemp"] = battery.pcbTemperatureC;
  document["soh"] = battery.stateOfHealthPercent;

  JsonArray cells = document["cells"].to<JsonArray>();
  for (uint8_t i = 0; i < battery.cellCount; ++i) {
    cells.add(battery.cellVoltages[i]);
  }

  JsonObject productObject = document["product"].to<JsonObject>();
  productObject["fw"] = product.firmwareVersion;
  productObject["mfr"] = product.manufacturerName;
  productObject["sn"] = product.serialNumber;

  JsonObject warningsObject = document["warnings"].to<JsonObject>();
  warningsObject["protection"] = warnings.hasActiveProtection;
  warningsObject["fault"] = warnings.hasFault;

  JsonObject gatewayObject = document["gateway"].to<JsonObject>();
  gatewayObject["wifi"] = gateway.wifiConnected;
  gatewayObject["ble"] = gateway.bleConnected;
  gatewayObject["ip"] = gateway.wifiIp;
  gatewayObject["bleAddress"] = gateway.bleAddress;
  gatewayObject["error"] = gateway.lastError;

  const size_t written = serializeJson(document, buffer, capacity);
  if (written == 0 || written >= capacity) {
    return 0;
  }
  return written;
}

}  // namespace wattcycle::telemetry
