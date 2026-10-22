#include "Display/P4OverviewView.h"

#include "Display/P4SolarRoute.h"

#include <cstdio>

namespace wattcycle::display {
namespace {

lv_obj_t* makeFlowRoot(lv_obj_t* parent) {
  lv_obj_t* root = lv_obj_create(parent);
  lv_obj_set_pos(root, 0, 0);
  lv_obj_set_size(root, 480, 800);
  lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(root, 0, 0);
  lv_obj_set_style_pad_all(root, 0, 0);
  lv_obj_set_style_radius(root, 0, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_CLICKABLE);
  return root;
}

void setWatts(lv_obj_t* label, bool active, int watts) {
  char line[24];
  if (active) {
    std::snprintf(line, sizeof(line), "%d W", watts);
  } else {
    std::snprintf(line, sizeof(line), "off");
  }
  lv_label_set_text(label, line);
}

}  // namespace

void P4OverviewView::mount(lv_obj_t* parent, const char* footer) {
  lv_obj_set_style_bg_color(parent, lv_color_hex(kBg), 0);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  flowRoot_ = makeFlowRoot(parent);
  mountGrid(flowRoot_);
  mountEco(flowRoot_);
  mountPorts(flowRoot_);
  mountPack(flowRoot_);
  mountSolar(flowRoot_);
  mountEdges(flowRoot_);
  footer_ = makeValueLabel(flowRoot_);
  lv_label_set_text(footer_, footer != nullptr ? footer : "");
  lv_obj_set_width(footer_, 448);
  lv_obj_set_style_text_align(footer_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(footer_, LV_ALIGN_TOP_MID, 0, 690);
  mountStatus(parent);
  mountMenu(parent);
  showPage(static_cast<uint8_t>(HubPage::Overview));
  setLinks(false, false);
  refreshClock();
}

void P4OverviewView::applyGrid(const P4OverviewFlow& flow) {
  setWatts(acIn_, flow.acInActive, flow.acInW);
  paintFlowNode(gridCard_, flow.acInActive);
  runLane(kLaneGrid, kSpineX, kGridY + kGridH + kLineGap, kSpineX, kEcoY - kLineGap, flow.acInActive);
}

void P4OverviewView::applyEco(const P4OverviewFlow& flow) {
  paintRadio(ecoCard_, flow.ecoLink);
  char line[48];
  if (flow.ecoValid) {
    std::snprintf(line, sizeof(line), "%u%%  %s", flow.ecoSoc, linkName(flow.ecoLink));
  } else {
    std::snprintf(line, sizeof(line), "--%%  %s", linkName(flow.ecoLink));
  }
  lv_label_set_text(ecoSoc_, line);
  if (flow.showEcoBattery) {
    std::snprintf(line, sizeof(line), "batt %+d W", flow.ecoBatteryW);
    const uint32_t color = flow.ecoBatteryW < 0 ? kDanger : kAccent;
    lv_obj_set_style_text_color(ecoBatt_, lv_color_hex(color), 0);
  } else {
    std::snprintf(line, sizeof(line), "batt --");
    lv_obj_set_style_text_color(ecoBatt_, lv_color_hex(kMuted), 0);
  }
  lv_label_set_text(ecoBatt_, line);
  if (flow.showBalance) {
    const int magnitude = flow.balanceW < 0 ? -flow.balanceW : flow.balanceW;
    const char* gap = flow.balanceW < 0 ? "out-in" : "in-out";
    std::snprintf(line, sizeof(line), "~%d W %s", magnitude, gap);
    const uint32_t color = flow.balanceW < 0 ? kBalanceNeg : kAccent;
    lv_obj_set_style_text_color(ecoBalance_, lv_color_hex(color), 0);
  } else {
    line[0] = '\0';
  }
  lv_label_set_text(ecoBalance_, line);
  if (flow.showSelf) {
    std::snprintf(line, sizeof(line), "~%d W self", flow.selfW);
  } else {
    line[0] = '\0';
  }
  lv_label_set_text(ecoSelf_, line);
}

void P4OverviewView::applyPorts(const P4OverviewFlow& flow) {
  setWatts(acOut_, flow.acOutOn, flow.acOutW);
  setWatts(dcOut_, flow.dcOutOn, flow.dcOutW);
  setWatts(usbOut_, flow.usbOutOn, flow.usbOutW);
  paintFlowNode(acCard_, flow.acOutOn);
  paintFlowNode(dcCard_, flow.dcOutOn);
  paintFlowNode(usbCard_, flow.usbOutOn);
  const int left = kColX + kColW + kLineGap;
  const int right = kPortX - kLineGap;
  runLane(kLaneAc, left, kEcoY + (kPortH / 2), right, kEcoY + (kPortH / 2), flow.acOutOn);
  runLane(kLaneDc, left, kEcoY + kPortPitch + (kPortH / 2), right, kEcoY + kPortPitch + (kPortH / 2), flow.dcOutOn);
  runLane(kLaneUsb, left, kEcoY + (2 * kPortPitch) + (kPortH / 2), right,
          kEcoY + (2 * kPortPitch) + (kPortH / 2), flow.usbOutOn);
}

void P4OverviewView::applyPack(const P4OverviewFlow& flow) {
  paintRadio(packCard_, flow.packLink);
  char line[48];
  if (flow.packValid) {
    std::snprintf(line, sizeof(line), "%u%%  %s", flow.packSoc, linkName(flow.packLink));
  } else {
    std::snprintf(line, sizeof(line), "--%%  %s", linkName(flow.packLink));
  }
  lv_label_set_text(packSoc_, line);
  if (flow.packEdge.active) {
    std::snprintf(line, sizeof(line), "%d W", flow.packWatts);
    lv_label_set_text(packEdge_, flow.packEdge.reverse ? "to Eco" : "to pack");
  } else {
    std::snprintf(line, sizeof(line), "idle");
    lv_label_set_text(packEdge_, "");
  }
  lv_label_set_text(packWatts_, line);
  const uint32_t color = flow.packWatts < 0 ? kDanger : kAccent;
  lv_obj_set_style_text_color(packWatts_, lv_color_hex(flow.packEdge.active ? color : kMuted), 0);
  const int top = kEcoY + kEcoH + kLineGap;
  const int bottom = kPackY - kLineGap;
  if (flow.packEdge.reverse) {
    runLane(kLanePack, kSpineX, bottom, kSpineX, top, flow.packEdge.active);
  } else {
    runLane(kLanePack, kSpineX, top, kSpineX, bottom, flow.packEdge.active);
  }
}

void P4OverviewView::applySolar(const P4OverviewFlow& flow) {
  paintRadio(solarCard_, flow.solarLink);
  char line[48];
  const P4SolarRoute route = solarRoute(flow);
  if (flow.solarActive) {
    std::snprintf(line, sizeof(line), "%d W  %s", flow.solarW, linkName(flow.solarLink));
  } else {
    std::snprintf(line, sizeof(line), "idle  %s", linkName(flow.solarLink));
  }
  lv_label_set_text(solarWatts_, line);
  if (route.toEco) {
    lv_label_set_text(solarEdge_, "solar to Eco");
  } else if (route.toPack) {
    lv_label_set_text(solarEdge_, "to pack");
  } else {
    lv_label_set_text(solarEdge_, "");
  }
  const int top = kPackY + kPackH + kLineGap;
  const int bottom = kSolarY - kLineGap;
  runLane(kLaneSolar, kSpineX, bottom, kSpineX, top, route.toPack);
  showBypass(route.toEco);
}

void P4OverviewView::apply(const P4OverviewFlow& flow) {
  applyGrid(flow);
  applyEco(flow);
  applyPorts(flow);
  applyPack(flow);
  applySolar(flow);
}

}  // namespace wattcycle::display
