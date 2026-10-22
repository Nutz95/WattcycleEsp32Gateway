#include "Display/P4OverviewView.h"

#include <cstdio>

namespace wattcycle::display {
namespace {

constexpr uint32_t kBg = 0x10141C;
constexpr uint32_t kCard = 0x1C2433;
constexpr uint32_t kInk = 0xE8EEFC;
constexpr uint32_t kMuted = 0x93A4C7;
constexpr uint32_t kAccent = 0x3ECF8E;
constexpr uint32_t kDanger = 0xFF6B6B;
constexpr uint32_t kSelf = 0xE6C07B;
constexpr uint32_t kBalanceNeg = 0xFB7185;

lv_obj_t* makeCard(lv_obj_t* parent, int y, int height) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_set_pos(card, 16, y);
  lv_obj_set_size(card, 448, height);
  lv_obj_set_style_bg_color(card, lv_color_hex(kCard), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(kDanger), 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  return card;
}

lv_obj_t* makeLabel(lv_obj_t* parent, int y, const char* text, uint32_t color) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_set_pos(label, 16, y);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  return label;
}

void paintLink(lv_obj_t* card, P4Link link) {
  const uint32_t color = link == P4Link::Up ? kAccent : kDanger;
  lv_obj_set_style_border_color(card, lv_color_hex(color), 0);
}

const char* linkName(P4Link link) {
  return link == P4Link::Up ? "live" : "no link";
}

}  // namespace

void P4OverviewView::mountEco(lv_obj_t* parent) {
  ecoCard_ = makeCard(parent, 48, 168);
  makeLabel(ecoCard_, 8, "EcoFlow", kInk);
  ecoSoc_ = makeLabel(ecoCard_, 36, "--%", kInk);
  ecoBatt_ = makeLabel(ecoCard_, 64, "batt", kMuted);
  ecoBalance_ = makeLabel(ecoCard_, 92, "", kMuted);
  ecoSelf_ = makeLabel(ecoCard_, 120, "", kSelf);
}

void P4OverviewView::mountPorts(lv_obj_t* parent) {
  lv_obj_t* card = makeCard(parent, 232, 168);
  paintLink(card, P4Link::Up);
  makeLabel(card, 8, "Ports", kInk);
  acIn_ = makeLabel(card, 36, "AC in", kMuted);
  acOut_ = makeLabel(card, 64, "AC out", kMuted);
  dcOut_ = makeLabel(card, 92, "DC out", kMuted);
  usbOut_ = makeLabel(card, 120, "USB", kMuted);
}

void P4OverviewView::mountPack(lv_obj_t* parent) {
  packCard_ = makeCard(parent, 416, 120);
  makeLabel(packCard_, 8, "Wattcycle", kInk);
  packSoc_ = makeLabel(packCard_, 40, "--%", kInk);
  packWatts_ = makeLabel(packCard_, 68, "idle", kMuted);
}

void P4OverviewView::mountSolar(lv_obj_t* parent) {
  solarCard_ = makeCard(parent, 552, 96);
  makeLabel(solarCard_, 8, "Solar", kInk);
  solarWatts_ = makeLabel(solarCard_, 40, "idle", kMuted);
}

void P4OverviewView::mount(lv_obj_t* parent, const char* footer) {
  lv_obj_set_style_bg_color(parent, lv_color_hex(kBg), 0);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  makeLabel(parent, 12, "Overview", kInk);
  mountEco(parent);
  mountPorts(parent);
  mountPack(parent);
  mountSolar(parent);
  makeLabel(parent, 668, footer != nullptr ? footer : "", kMuted);
}

void P4OverviewView::applyEco(const P4OverviewFlow& flow) {
  paintLink(ecoCard_, flow.ecoLink);
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
  char line[48];
  std::snprintf(line, sizeof(line), "AC in %d W%s", flow.acInW, flow.acInActive ? "" : " off");
  lv_label_set_text(acIn_, line);
  std::snprintf(line, sizeof(line), "AC out %d W %s", flow.acOutW, flow.acOutOn ? "on" : "off");
  lv_label_set_text(acOut_, line);
  std::snprintf(line, sizeof(line), "DC out %d W %s", flow.dcOutW, flow.dcOutOn ? "on" : "off");
  lv_label_set_text(dcOut_, line);
  std::snprintf(line, sizeof(line), "USB %d W %s", flow.usbOutW, flow.usbOutOn ? "on" : "off");
  lv_label_set_text(usbOut_, line);
}

void P4OverviewView::applyPack(const P4OverviewFlow& flow) {
  paintLink(packCard_, flow.packLink);
  char line[48];
  if (flow.packValid) {
    std::snprintf(line, sizeof(line), "%u%%  %s", flow.packSoc, linkName(flow.packLink));
  } else {
    std::snprintf(line, sizeof(line), "--%%  %s", linkName(flow.packLink));
  }
  lv_label_set_text(packSoc_, line);
  if (!flow.packEdge.active) {
    std::snprintf(line, sizeof(line), "idle");
  } else if (flow.packEdge.reverse) {
    std::snprintf(line, sizeof(line), "%d W to EcoFlow", flow.packWatts);
  } else {
    std::snprintf(line, sizeof(line), "%+d W from EcoFlow", flow.packWatts);
  }
  lv_label_set_text(packWatts_, line);
}

void P4OverviewView::applySolar(const P4OverviewFlow& flow) {
  paintLink(solarCard_, flow.solarLink);
  char line[48];
  if (flow.solarActive) {
    std::snprintf(line, sizeof(line), "%d W  %s", flow.solarW, linkName(flow.solarLink));
  } else {
    std::snprintf(line, sizeof(line), "idle  %s", linkName(flow.solarLink));
  }
  lv_label_set_text(solarWatts_, line);
}

void P4OverviewView::apply(const P4OverviewFlow& flow) {
  applyEco(flow);
  applyPorts(flow);
  applyPack(flow);
  applySolar(flow);
}

}  // namespace wattcycle::display
