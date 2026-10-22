#pragma once

namespace wattcycle::panel {

/// One-shot coprocessor update. Replaces the C6 image with an ESP-NOW-capable slave.
class P4C6Update {
 public:
  /// Begin the download. Safe to call once, after the STA has an address.
  void start();
  /// Advance the transfer. True when the coprocessor has rebooted into the new image.
  bool loop();
  /// True after a failed attempt. The radio should not retry forever.
  bool failed() const;

 private:
  bool started_ = false;
  bool finished_ = false;
  bool failed_ = false;
  bool writing_ = false;
};

}  // namespace wattcycle::panel
