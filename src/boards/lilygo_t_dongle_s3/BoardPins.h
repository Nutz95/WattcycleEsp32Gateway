#pragma once

#include "boards/dongle_s3/ActiveBoardPins.h"

/// Compatibility shim: historical namespace used by BMS/EcoFlow bridges.
/// Pin values come from ActiveBoardPins (Pocket by default, LilyGO via flag).
namespace boards::lilygo_t_dongle_s3 {

using boards::dongle_s3::kBacklightActiveHigh;
using boards::dongle_s3::kBacklightGpio;
using boards::dongle_s3::kBoardName;
using boards::dongle_s3::kButtonGpio;
using boards::dongle_s3::kDisplayHeight;
using boards::dongle_s3::kDisplayWidth;
using boards::dongle_s3::kLedClockGpio;
using boards::dongle_s3::kLedDataGpio;
using boards::dongle_s3::kLongPressMs;
using boards::dongle_s3::kTftCs;
using boards::dongle_s3::kTftDc;
using boards::dongle_s3::kTftMosi;
using boards::dongle_s3::kTftPortraitHeight;
using boards::dongle_s3::kTftPortraitWidth;
using boards::dongle_s3::kTftRst;
using boards::dongle_s3::kTftSclk;

}  // namespace boards::lilygo_t_dongle_s3
