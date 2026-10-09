#pragma once

#include "Config/AppConfig.h"

namespace ecoflow::config {

/// Loads `AppConfig` from PlatformIO build flags / env injection.
class AppConfigFactory {
 public:
  /// Load bridge config from PlatformIO build flags / env injection.
  static AppConfig fromBuildFlags();
};

}  // namespace ecoflow::config
