#include "P4MicLevel.h"

namespace wattcycle::panel {

int P4MicLevel::percent() const {
  return percent_;
}

void P4MicLevel::setPercent(int percent) {
  if (percent < 0) {
    percent = 0;
  }
  if (percent > 100) {
    percent = 100;
  }
  percent_ = percent;
}

}  // namespace wattcycle::panel
