#pragma once

#include "Auth/IGuestAccess.h"

namespace wattcycle::panel {

/// Guest flag in `/guest.cfg` on the SD card. A missing file means off.
class P4GuestAccess : public auth::IGuestAccess {
 public:
  void begin();
  bool enabled() const override;
  uint32_t tabs() const override;
  bool setEnabled(bool on) override;
  bool setTabs(uint32_t tabs) override;

 private:
  bool store();

  bool enabled_ = false;
  uint32_t tabs_ = kDefaultTabs;
};

}  // namespace wattcycle::panel
