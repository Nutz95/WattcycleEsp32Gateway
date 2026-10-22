#pragma once

namespace wattcycle::panel {

/// Copies the flashed site onto the SD card, then unmounts LittleFS.
class P4WebPublish {
 public:
  /// True when /index.html is on the SD and LittleFS is no longer mounted.
  bool install();
};

}  // namespace wattcycle::panel
