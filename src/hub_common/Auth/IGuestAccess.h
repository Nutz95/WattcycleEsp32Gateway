#pragma once

#include <cstdint>

namespace wattcycle::auth {

/// Guest read switch stored by the panel. Off unless a composition root enables it.
class IGuestAccess {
 public:
  virtual ~IGuestAccess() = default;

  /// Eleven dashboard tabs, in web menu order. Camera and plugs stay signed-in.
  static constexpr uint32_t kTabCount = 11;
  static constexpr uint32_t kTabOverview = 1u << 0;
  static constexpr uint32_t kTabDashboard = 1u << 1;
  static constexpr uint32_t kTabSolar = 1u << 2;
  static constexpr uint32_t kTabWattcycle = 1u << 3;
  static constexpr uint32_t kTabEcoFlow = 1u << 4;
  static constexpr uint32_t kTabHistory = 1u << 5;
  static constexpr uint32_t kTabCells = 1u << 6;
  static constexpr uint32_t kTabTemperatures = 1u << 7;
  static constexpr uint32_t kTabWarnings = 1u << 8;
  static constexpr uint32_t kTabGateway = 1u << 9;
  static constexpr uint32_t kTabEsp = 1u << 10;
  /// Every defined tab bit. A stored mask is clipped to this.
  static constexpr uint32_t kTabMask = (1u << kTabCount) - 1u;
  /// A new guest config grants no tabs until a signed-in user selects some.
  static constexpr uint32_t kDefaultTabs = 0;

  /// True when a signed-out browser may read telemetry and history.
  virtual bool enabled() const = 0;
  /// Bit mask of the dashboard tabs a guest may open.
  virtual uint32_t tabs() const = 0;
  /// Persist the switch. Keeps the current tab mask. False if storage fails.
  virtual bool setEnabled(bool on) = 0;
  /// Persist which dashboard tabs a guest may open. False if storage fails.
  virtual bool setTabs(uint32_t tabs) = 0;
  /// Persist the switch and the tab mask in one write. False if storage fails.
  virtual bool update(bool enabled, uint32_t tabs) = 0;
};

}  // namespace wattcycle::auth
