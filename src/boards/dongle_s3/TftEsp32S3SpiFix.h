#pragma once

/// Force TFT_eSPI-compatible SPI base map on ESP32-S3.
/// IDF defines REG_SPI_BASE(i)=0 for i<2, so SPI_PORT=FSPI(1) writes to 0x10
/// (StoreProhibited). Vendor TFT_eSPI uses this mapping so FSPI(1) → GPSPI2.
/// Match vendor demos: do NOT also define USE_FSPI_PORT (that sets SPI_PORT=2
/// and can desync SPIClass vs register macros depending on IDF headers).
#include "soc/soc.h"

#ifdef REG_SPI_BASE
#undef REG_SPI_BASE
#endif
#define REG_SPI_BASE(i) (((i) > 1) ? (DR_REG_SPI3_BASE) : (DR_REG_SPI2_BASE))
