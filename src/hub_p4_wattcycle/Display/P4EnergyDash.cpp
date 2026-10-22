#include "Display/P4EnergyDash.h"

#include "Display/P4OverviewStyle.h"

namespace wattcycle::display {
namespace {

void moveEnergyBead(void* var, int32_t value) {
  auto* dash = static_cast<P4EnergyDash*>(var);
  const int x = dash->fromX + ((dash->toX - dash->fromX) * value) / 1000;
  const int y = dash->fromY + ((dash->toY - dash->fromY) * value) / 1000;
  int spanX = dash->toX - dash->fromX;
  int spanY = dash->toY - dash->fromY;
  if (spanX < 0) {
    spanX = -spanX;
  }
  if (spanY < 0) {
    spanY = -spanY;
  }
  const bool vertical = spanY > spanX;
  const int along = dash->along > 2 ? dash->along : 3;
  const int width = vertical ? 4 : along;
  const int height = vertical ? along : 4;
  lv_obj_set_size(dash->bead, width, height);
  lv_obj_set_pos(dash->bead, x - (width / 2), y - (height / 2));
}

bool sameRun(const P4EnergyDash& dash, int fromX, int fromY, int toX, int toY, bool active) {
  return dash.running && active && dash.fromX == fromX && dash.fromY == fromY && dash.toX == toX &&
         dash.toY == toY;
}

}  // namespace

void mountEnergyDash(P4EnergyDash& dash, lv_obj_t* parent) {
  dash.bead = lv_obj_create(parent);
  lv_obj_set_size(dash.bead, 16, 5);
  lv_obj_set_style_bg_color(dash.bead, lv_color_hex(kAccent), 0);
  lv_obj_set_style_bg_opa(dash.bead, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(dash.bead, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(dash.bead, 0, 0);
  lv_obj_set_style_pad_all(dash.bead, 0, 0);
  lv_obj_clear_flag(dash.bead, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(dash.bead, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(dash.bead, LV_OBJ_FLAG_HIDDEN);
}

void fitDash(int& fromX, int& fromY, int& toX, int& toY, int& along) {
  int dx = toX - fromX;
  int dy = toY - fromY;
  int adx = dx < 0 ? -dx : dx;
  int ady = dy < 0 ? -dy : dy;
  const bool vertical = ady > adx;
  const int span = vertical ? ady : adx;
  along = 12;
  if (span < 36) {
    along = 8;
  }
  if (span < 22) {
    along = 5;
  }
  int inset = (along / 2) + 2;
  if ((inset * 2) >= span) {
    inset = span / 3;
    along = span - (2 * inset);
    if (along < 3) {
      along = 3;
    }
  }
  if (vertical) {
    const int sign = dy > 0 ? 1 : -1;
    fromY += sign * inset;
    toY -= sign * inset;
    return;
  }
  if (adx == 0) {
    return;
  }
  const int sign = dx > 0 ? 1 : -1;
  fromX += sign * inset;
  toX -= sign * inset;
}

void runEnergyDash(P4EnergyDash& dash, int fromX, int fromY, int toX, int toY, bool active, uint32_t delayMs) {
  int along = 10;
  fitDash(fromX, fromY, toX, toY, along);
  if (sameRun(dash, fromX, fromY, toX, toY, active) && dash.along == along) {
    return;
  }
  dash.along = along;
  lv_anim_del(&dash, moveEnergyBead);
  dash.fromX = static_cast<lv_coord_t>(fromX);
  dash.fromY = static_cast<lv_coord_t>(fromY);
  dash.toX = static_cast<lv_coord_t>(toX);
  dash.toY = static_cast<lv_coord_t>(toY);
  dash.running = false;
  if (!active) {
    lv_obj_add_flag(dash.bead, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(dash.bead, LV_OBJ_FLAG_HIDDEN);
  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, &dash);
  lv_anim_set_exec_cb(&anim, moveEnergyBead);
  lv_anim_set_values(&anim, 0, 1000);
  lv_anim_set_time(&anim, 1400);
  lv_anim_set_delay(&anim, delayMs);
  lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&anim, lv_anim_path_linear);
  lv_anim_start(&anim);
  dash.running = true;
}

}  // namespace wattcycle::display
