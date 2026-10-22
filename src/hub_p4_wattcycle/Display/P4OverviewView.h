#pragma once

#include "Display/HubPage.h"
#include "Display/P4EnergyDash.h"
#include "Display/P4OverviewStyle.h"

namespace wattcycle::display {

/// LVGL shell for the 480×800 panel: status band, energy flow, scrolling page menu.
class P4OverviewView {
 public:
  /// Build the shell on `parent`. `footer` is the one-line demo note.
  void mount(lv_obj_t* parent, const char* footer);
  /// Refresh node text, borders, and the direction of each energy lane.
  void apply(const P4OverviewFlow& flow);
  /// Paint Wi-Fi and the ESP link. Both stay down until those radios exist here.
  void setLinks(bool wifiUp, bool espUp);
  /// Color the SD icon from free space. A missing card is red with a mark.
  void setStorage(bool ready, int usedMb, int totalMb);
  /// Clock and date. A year before 2024 stays blank.
  void refreshClock();
  /// Highlight one menu icon and show that page.
  void showPage(uint8_t index);
  /// Called after the menu changes page. The panel uses it to show the matching screen.
  void setPageHandler(void (*handler)(uint8_t index, void* user), void* user);

 private:
  void mountGrid(lv_obj_t* parent);
  void mountEco(lv_obj_t* parent);
  void mountPorts(lv_obj_t* parent);
  void mountPack(lv_obj_t* parent);
  void mountSolar(lv_obj_t* parent);
  void mountEdges(lv_obj_t* parent);
  void mountStatus(lv_obj_t* parent);
  void mountMenu(lv_obj_t* parent);
  void mountLane(int lane, int x1, int y1, int x2, int y2);
  void mountTransitLabels();
  void runLane(int lane, int x1, int y1, int x2, int y2, bool active);
  void showBypass(bool active);
  void placeSolarWatts(bool alongBypass);
  void applyGrid(const P4OverviewFlow& flow);
  void applyEco(const P4OverviewFlow& flow);
  void applyPorts(const P4OverviewFlow& flow);
  void applyPack(const P4OverviewFlow& flow);
  void applySolar(const P4OverviewFlow& flow);
  void styleMenu(uint8_t index);

  lv_obj_t* flowRoot_ = nullptr;
  lv_obj_t* placeholder_ = nullptr;
  lv_obj_t* footer_ = nullptr;
  lv_obj_t* wifiIcon_ = nullptr;
  lv_obj_t* espIcon_ = nullptr;
  lv_obj_t* sdIcon_ = nullptr;
  lv_obj_t* sdWarn_ = nullptr;
  lv_obj_t* clock_ = nullptr;
  lv_obj_t* date_ = nullptr;
  lv_obj_t* menu_ = nullptr;
  lv_obj_t* menuBtn_[kHubPageCount] = {};
  lv_obj_t* menuIcon_[kHubPageCount] = {};
  lv_obj_t* gridCard_ = nullptr;
  lv_obj_t* acIn_ = nullptr;
  lv_obj_t* ecoCard_ = nullptr;
  lv_obj_t* ecoSoc_ = nullptr;
  lv_obj_t* ecoBatt_ = nullptr;
  lv_obj_t* ecoBalance_ = nullptr;
  lv_obj_t* ecoSelf_ = nullptr;
  lv_obj_t* acCard_ = nullptr;
  lv_obj_t* acOut_ = nullptr;
  lv_obj_t* dcCard_ = nullptr;
  lv_obj_t* dcOut_ = nullptr;
  lv_obj_t* usbCard_ = nullptr;
  lv_obj_t* usbOut_ = nullptr;
  lv_obj_t* packCard_ = nullptr;
  lv_obj_t* packSoc_ = nullptr;
  lv_obj_t* packWatts_ = nullptr;
  lv_obj_t* solarCard_ = nullptr;
  lv_obj_t* solarWatts_ = nullptr;
  lv_obj_t* packEdge_ = nullptr;
  lv_obj_t* solarEdge_ = nullptr;
  lv_obj_t* bypassIn_ = nullptr;
  lv_obj_t* bypassOut_ = nullptr;
  lv_obj_t* lanes_[kLaneCount] = {};
  lv_point_t lanePts_[kLaneCount][2] = {};
  lv_point_t bypassInPts_[2] = {};
  lv_point_t bypassOutPts_[2] = {};
  P4EnergyDash beads_[kLaneCount][kBeadsPerLane] = {};
  uint8_t page_ = 0;
  bool wifiUp_ = false;
  bool espUp_ = false;
  int clockMinute_ = -2;
  void (*pageHandler_)(uint8_t index, void* user) = nullptr;
  void* pageUser_ = nullptr;
};

}  // namespace wattcycle::display
