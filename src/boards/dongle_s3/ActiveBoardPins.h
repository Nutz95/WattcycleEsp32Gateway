#pragma once

// Select dongle pin map at compile time:
//   default / -DBOARD_DONGLE_POCKET=1  → Pocket-Dongle-S3-0.96
//   -DBOARD_DONGLE_LILYGO=1            → LilyGO T-Dongle-S3
#if defined(BOARD_DONGLE_LILYGO)
#include "boards/dongle_s3/LilygoTdongleS3Pins.h"
#else
#include "boards/dongle_s3/PocketDongle096Pins.h"
#endif
