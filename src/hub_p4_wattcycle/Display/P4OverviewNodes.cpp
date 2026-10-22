#include "Display/P4OverviewView.h"

#include <cstdint>

namespace wattcycle::display {
namespace {

void onEnergyCardTap(lv_event_t* event) {
  auto* view = static_cast<P4OverviewView*>(lv_event_get_user_data(event));
  const auto page = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target(event)));
  if (view != nullptr) {
    view->showPage(static_cast<uint8_t>(page));
  }
}

void armEnergyCard(P4OverviewView* view, lv_obj_t* card, HubPage page) {
  if (view == nullptr || card == nullptr) {
    return;
  }
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  const uint32_t children = lv_obj_get_child_cnt(card);
  for (uint32_t child = 0; child < children; ++child) {
    lv_obj_clear_flag(lv_obj_get_child(card, child), LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_set_user_data(card, reinterpret_cast<void*>(static_cast<uintptr_t>(page)));
  lv_obj_add_event_cb(card, onEnergyCardTap, LV_EVENT_CLICKED, view);
}

}  // namespace

void P4OverviewView::mountGrid(lv_obj_t* parent) {
  gridCard_ = makeCard(parent, kGridX, kGridY, kGridW, kGridH);
  layoutCardText(gridCard_, "Grid", nullptr, 0);
}

void P4OverviewView::mountEco(lv_obj_t* parent) {
  ecoCard_ = makeCard(parent, kColX, kEcoY, kColW, kEcoH);
  ecoSoc_ = makeValueLabel(ecoCard_);
  ecoBatt_ = makeValueLabel(ecoCard_);
  ecoBalance_ = makeValueLabel(ecoCard_);
  ecoSelf_ = makeValueLabel(ecoCard_);
  lv_obj_set_style_text_color(ecoSelf_, lv_color_hex(kSelf), 0);
  lv_obj_t* values[] = {ecoSoc_, ecoBatt_, ecoBalance_, ecoSelf_};
  layoutCardText(ecoCard_, "EcoFlow", values, 4);
  armEnergyCard(this, ecoCard_, HubPage::EcoFlow);
}

void P4OverviewView::mountPorts(lv_obj_t* parent) {
  acCard_ = makeCard(parent, kPortX, kEcoY, kPortW, kPortH);
  layoutCardText(acCard_, "AC out", nullptr, 0);
  dcCard_ = makeCard(parent, kPortX, kEcoY + kPortPitch, kPortW, kPortH);
  layoutCardText(dcCard_, "DC out", nullptr, 0);
  usbCard_ = makeCard(parent, kPortX, kEcoY + (2 * kPortPitch), kPortW, kPortH);
  layoutCardText(usbCard_, "USB", nullptr, 0);
}

void P4OverviewView::mountPack(lv_obj_t* parent) {
  packCard_ = makeCard(parent, kColX, kPackY, kColW, kPackH);
  packSoc_ = makeValueLabel(packCard_);
  lv_obj_t* values[] = {packSoc_};
  layoutCardText(packCard_, "Wattcycle", values, 1);
  armEnergyCard(this, packCard_, HubPage::Wattcycle);
}

void P4OverviewView::mountSolar(lv_obj_t* parent) {
  solarCard_ = makeCard(parent, kColX, kSolarY, kColW, kSolarH);
  layoutCardText(solarCard_, "Solar", nullptr, 0);
  armEnergyCard(this, solarCard_, HubPage::Solar);
}

}  // namespace wattcycle::display
