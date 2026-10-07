#pragma once

#include <cstdint>

namespace wattcycle::display {

enum class DisplayAuthKind : uint8_t {
  None = 0,
  ConfirmSetup,
  ConfirmReset,
};

/// Display-owned auth UI payload (CompositionRoot maps from Auth domain).
struct DisplayAuthPrompt {
  DisplayAuthKind kind = DisplayAuthKind::None;
  char username[33] = {};
};

}  // namespace wattcycle::display
