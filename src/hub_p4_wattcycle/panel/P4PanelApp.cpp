#include "P4PanelApp.h"

#include <Arduino.h>

#include "Display/P4OverviewView.h"
#include "esp_heap_caps.h"
#include "driver/i2c_master.h"
#include "lvgl.h"
#include "pins_config.h"
#include "src/lcd/st7701_lcd.h"
#include "src/touch/gt911_touch.h"

namespace wattcycle::panel {
namespace {

bsp_lcd_handles_t lcdPanels;
st7701_lcd lcd(LCD_RST);
gt911_touch touch(TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);
lv_disp_draw_buf_t drawBuf;
lv_color_t* frameA = nullptr;
lv_color_t* frameB = nullptr;
wattcycle::display::P4OverviewView overview;

bool onFlushDone(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t* event, void* user) {
  (void)panel;
  (void)event;
  auto* drv = static_cast<lv_disp_drv_t*>(user);
  if (drv != nullptr) {
    lv_disp_flush_ready(drv);
  }
  return false;
}

void flushDisplay(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color) {
  (void)disp;
  lcd.lcd_draw_bitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, &color->full);
}

void readTouch(lv_indev_drv_t* driver, lv_indev_data_t* data) {
  (void)driver;
  uint16_t x = 0;
  uint16_t y = 0;
  if (!touch.getTouch(&x, &y)) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  data->state = LV_INDEV_STATE_PR;
  data->point.x = x;
  data->point.y = y;
}

display::P4OverviewFlow demoFlow() {
  display::P4OverviewFlow flow;
  flow.packLink = display::P4Link::Up;
  flow.ecoLink = display::P4Link::Up;
  flow.solarLink = display::P4Link::Down;
  flow.packValid = true;
  flow.packSoc = 80;
  flow.packWatts = -71;
  flow.packEdge.active = true;
  flow.packEdge.reverse = true;
  flow.ecoValid = true;
  flow.ecoSoc = 18;
  flow.showEcoBattery = true;
  flow.ecoBatteryW = -71;
  flow.showBalance = true;
  flow.balanceW = -22;
  flow.showSelf = true;
  flow.selfW = 49;
  flow.acOutOn = true;
  flow.acOutW = 93;
  return flow;
}

bool startPanel() {
  i2c_master_bus_config_t busConfig = {};
  busConfig.i2c_port = I2C_NUM_1;
  busConfig.sda_io_num = static_cast<gpio_num_t>(TP_I2C_SDA);
  busConfig.scl_io_num = static_cast<gpio_num_t>(TP_I2C_SCL);
  busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
  busConfig.glitch_ignore_cnt = 7;
  busConfig.flags.enable_internal_pullup = 1;
  i2c_master_bus_handle_t bus = nullptr;
  i2c_new_master_bus(&busConfig, &bus);
  lcd.begin();
  touch.begin();
  lcd.get_handle(&lcdPanels);
  return lcdPanels.panel != nullptr;
}

bool startLvgl() {
  lv_init();
  const size_t bufferSize = sizeof(lv_color_t) * LCD_H_RES * LCD_V_RES;
  frameA = static_cast<lv_color_t*>(heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM));
  frameB = static_cast<lv_color_t*>(heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM));
  if (frameA == nullptr || frameB == nullptr) {
    return false;
  }
  lv_disp_draw_buf_init(&drawBuf, frameA, frameB, LCD_H_RES * LCD_V_RES);
  static lv_disp_drv_t dispDrv;
  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = LCD_H_RES;
  dispDrv.ver_res = LCD_V_RES;
  dispDrv.flush_cb = flushDisplay;
  dispDrv.draw_buf = &drawBuf;
  lv_disp_drv_register(&dispDrv);
  static lv_indev_drv_t indevDrv;
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = readTouch;
  lv_indev_drv_register(&indevDrv);
  esp_lcd_dpi_panel_event_callbacks_t callbacks = {};
  callbacks.on_color_trans_done = onFlushDone;
  esp_lcd_dpi_panel_register_event_callbacks(lcdPanels.panel, &callbacks, &dispDrv);
  return true;
}

}  // namespace

void setupPanel() {
  Serial.begin(115200);
  if (!startPanel() || !startLvgl()) {
    Serial.println("panel start failed");
    return;
  }
  overview.mount(lv_scr_act(), "demo - bridges stay on the M5");
  overview.apply(demoFlow());
  Serial.println("overview ready");
}

void loopPanel() {
  lv_timer_handler();
  delay(5);
}

}  // namespace wattcycle::panel
