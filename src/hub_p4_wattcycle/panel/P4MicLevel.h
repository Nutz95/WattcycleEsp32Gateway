#pragma once

#include "Audio/IMicGain.h"

namespace wattcycle::panel {

/// RAM microphone level. Default is a step under the old fixed gain of 6.
class P4MicLevel : public audio::IMicGain {
 public:
  int percent() const override;
  void setPercent(int percent) override;

 private:
  volatile int percent_ = 80;
};

}  // namespace wattcycle::panel
