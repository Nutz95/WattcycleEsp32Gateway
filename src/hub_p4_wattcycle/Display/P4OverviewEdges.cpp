#include "Display/P4OverviewView.h"

namespace wattcycle::display {
namespace {

void setVisible(lv_obj_t* obj, bool visible) {
  if (visible) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

lv_obj_t* makeTransit(lv_obj_t* parent, int x, int y) {
  lv_obj_t* label = makeValueLabel(parent);
  lv_obj_set_width(label, 84);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_pos(label, x, y);
  lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
  return label;
}

int gapMidY(int top, int bottom) {
  return ((top + bottom) / 2) - 8;
}

}  // namespace

void P4OverviewView::mountLane(int lane, int x1, int y1, int x2, int y2) {
  lanePts_[lane][0].x = static_cast<lv_coord_t>(x1);
  lanePts_[lane][0].y = static_cast<lv_coord_t>(y1);
  lanePts_[lane][1].x = static_cast<lv_coord_t>(x2);
  lanePts_[lane][1].y = static_cast<lv_coord_t>(y2);
  lanes_[lane] = makeEdge(flowRoot_, lanePts_[lane]);
  mountEnergyDash(beads_[lane][0], flowRoot_);
  mountEnergyDash(beads_[lane][1], flowRoot_);
}

void P4OverviewView::runLane(int lane, int x1, int y1, int x2, int y2, bool active) {
  paintEnergy(lanes_[lane], active);
  runEnergyDash(beads_[lane][0], x1, y1, x2, y2, active, 0);
  runEnergyDash(beads_[lane][1], x1, y1, x2, y2, active, 700);
}

void P4OverviewView::showBypass(bool active) {
  setVisible(lanes_[kLaneBypass], active);
  setVisible(bypassIn_, active);
  setVisible(bypassOut_, active);
  paintEnergy(bypassIn_, active);
  paintEnergy(bypassOut_, active);
  runLane(kLaneBypass, kRailX, kSolarMidY, kRailX, kEcoMidY, active);
}

void P4OverviewView::mountEdges(lv_obj_t* parent) {
  (void)parent;
  const int columnRight = kColX + kColW;
  mountLane(kLaneGrid, kSpineX, kGridY + kGridH + kLineGap, kSpineX, kEcoY - kLineGap);
  mountLane(kLaneAc, columnRight + kLineGap, kEcoY + (kPortH / 2), kPortX - kLineGap, kEcoY + (kPortH / 2));
  mountLane(kLaneDc, columnRight + kLineGap, kEcoY + kPortPitch + (kPortH / 2), kPortX - kLineGap,
            kEcoY + kPortPitch + (kPortH / 2));
  mountLane(kLaneUsb, columnRight + kLineGap, kEcoY + (2 * kPortPitch) + (kPortH / 2), kPortX - kLineGap,
            kEcoY + (2 * kPortPitch) + (kPortH / 2));
  mountLane(kLanePack, kSpineX, kEcoY + kEcoH + kLineGap, kSpineX, kPackY - kLineGap);
  mountLane(kLaneSolar, kSpineX, kPackY + kPackH + kLineGap, kSpineX, kSolarY - kLineGap);
  mountLane(kLaneBypass, kRailX, kSolarMidY, kRailX, kEcoMidY);
  bypassInPts_[0].x = static_cast<lv_coord_t>(kRailX);
  bypassInPts_[0].y = static_cast<lv_coord_t>(kSolarMidY);
  bypassInPts_[1].x = static_cast<lv_coord_t>(kColX);
  bypassInPts_[1].y = static_cast<lv_coord_t>(kSolarMidY);
  bypassOutPts_[0].x = static_cast<lv_coord_t>(kRailX);
  bypassOutPts_[0].y = static_cast<lv_coord_t>(kEcoMidY);
  bypassOutPts_[1].x = static_cast<lv_coord_t>(kColX);
  bypassOutPts_[1].y = static_cast<lv_coord_t>(kEcoMidY);
  bypassIn_ = makeEdge(flowRoot_, bypassInPts_);
  bypassOut_ = makeEdge(flowRoot_, bypassOutPts_);
  setVisible(lanes_[kLaneBypass], false);
  setVisible(bypassIn_, false);
  setVisible(bypassOut_, false);
  packEdge_ = makeValueLabel(flowRoot_);
  solarEdge_ = makeValueLabel(flowRoot_);
  lv_obj_add_flag(packEdge_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(solarEdge_, LV_OBJ_FLAG_HIDDEN);
  mountTransitLabels();
}

void P4OverviewView::mountTransitLabels() {
  const int beside = kSpineX + 14;
  const int gridTop = kGridY + kGridH + kLineGap;
  const int gridBottom = kEcoY - kLineGap;
  acIn_ = makeTransit(flowRoot_, beside, gapMidY(gridTop, gridBottom));
  const int portLeft = kColX + kColW + kLineGap;
  const int portRight = kPortX - kLineGap;
  const int portX = ((portLeft + portRight) / 2) - 28;
  acOut_ = makeTransit(flowRoot_, portX, kEcoY + (kPortH / 2) - 16);
  dcOut_ = makeTransit(flowRoot_, portX, kEcoY + kPortPitch + (kPortH / 2) - 16);
  usbOut_ = makeTransit(flowRoot_, portX, kEcoY + (2 * kPortPitch) + (kPortH / 2) - 16);
  const int packTop = kEcoY + kEcoH + kLineGap;
  const int packBottom = kPackY - kLineGap;
  packWatts_ = makeTransit(flowRoot_, beside, gapMidY(packTop, packBottom));
  solarWatts_ = makeTransit(flowRoot_, beside, gapMidY(kPackY + kPackH + kLineGap, kSolarY - kLineGap));
}

void P4OverviewView::placeSolarWatts(bool alongBypass) {
  if (solarWatts_ == nullptr) {
    return;
  }
  if (alongBypass) {
    lv_obj_set_pos(solarWatts_, kRailX + 16, kEcoMidY - 18);
    return;
  }
  const int y = gapMidY(kPackY + kPackH + kLineGap, kSolarY - kLineGap);
  lv_obj_set_pos(solarWatts_, kSpineX + 14, y);
}

}  // namespace wattcycle::display
