#pragma once

#include <cstdint>

namespace wattcycle::auth {

/// Guest read switch stored by the panel. Off unless a composition root enables it.
class IGuestAccess {
 public:
  virtual ~IGuestAccess() = default;

  /// Eleven dashboard tabs, in menu order. The camera is never one of them.
  static constexpr uint32_t kTabCount = 11;
  static constexpr uint32_t kDefaultTabs = (1u << kTabCount) - 1u;

  /// True when a signed-out browser may read telemetry and history.
  virtual bool enabled() const = 0;
  /// Bit mask of the dashboard tabs a guest may open.
  virtual uint32_t tabs() const = 0;
  /// Persist the switch. Keeps the current tab mask. False if storage fails.
  virtual bool setEnabled(bool on) = 0;
  /// Persist which dashboard tabs a guest may open. False if storage fails.
  virtual bool setTabs(uint32_t tabs) = 0;
};

}  // namespace wattcycle::auth
