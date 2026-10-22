#include "Display/P4OverviewView.h"

namespace wattcycle::display {

void P4OverviewView::mountGrid(lv_obj_t* parent) {
  gridCard_ = makeCard(parent, kGridX, kGridY, kGridW, kGridH);
  acIn_ = makeValueLabel(gridCard_);
  lv_obj_t* values[] = {acIn_};
  layoutCardText(gridCard_, "Grid", values, 1);
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
}

void P4OverviewView::mountPorts(lv_obj_t* parent) {
  acCard_ = makeCard(parent, kPortX, kEcoY, kPortW, kPortH);
  acOut_ = makeValueLabel(acCard_);
  lv_obj_t* acValues[] = {acOut_};
  layoutCardText(acCard_, "AC out", acValues, 1);
  dcCard_ = makeCard(parent, kPortX, kEcoY + kPortPitch, kPortW, kPortH);
  dcOut_ = makeValueLabel(dcCard_);
  lv_obj_t* dcValues[] = {dcOut_};
  layoutCardText(dcCard_, "DC out", dcValues, 1);
  usbCard_ = makeCard(parent, kPortX, kEcoY + (2 * kPortPitch), kPortW, kPortH);
  usbOut_ = makeValueLabel(usbCard_);
  lv_obj_t* usbValues[] = {usbOut_};
  layoutCardText(usbCard_, "USB", usbValues, 1);
}

void P4OverviewView::mountPack(lv_obj_t* parent) {
  packCard_ = makeCard(parent, kColX, kPackY, kColW, kPackH);
  packSoc_ = makeValueLabel(packCard_);
  packWatts_ = makeValueLabel(packCard_);
  lv_obj_t* values[] = {packSoc_, packWatts_};
  layoutCardText(packCard_, "Wattcycle", values, 2);
}

void P4OverviewView::mountSolar(lv_obj_t* parent) {
  solarCard_ = makeCard(parent, kColX, kSolarY, kColW, kSolarH);
  solarWatts_ = makeValueLabel(solarCard_);
  lv_obj_t* values[] = {solarWatts_};
  layoutCardText(solarCard_, "Solar", values, 1);
}

}  // namespace wattcycle::display
