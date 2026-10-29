#pragma once

#include <cstdint>

namespace wattcycle::storage {

/// Column order for `/history/.../YYYY-MM-DD.csv` (must match HistoryDayFile header).
enum class HistoryCsvColumn : uint8_t {
  Epoch = 0,
  Soc,
  PackV,
  PackA,
  PackW,
  SolarV,
  SolarA,
  SolarW,
  PackOk,
  SolarOk,
  PackAh,
  MosC,
  PcbC,
  SolarWh,
  SolarAh,
  EcoOk,
  EcoSoc,
  EcoAcOut,
  EcoAcIn,
  EcoDc,
  EcoUsb,
  Count
};

/// Map a CSV field index to a named column. False when out of range.
inline bool historyCsvColumnAt(int index, HistoryCsvColumn& out) {
  if (index < 0 || index >= static_cast<int>(HistoryCsvColumn::Count)) {
    return false;
  }
  out = static_cast<HistoryCsvColumn>(index);
  return true;
}

}  // namespace wattcycle::storage
