#include "P4PanelApp.h"

#include <Arduino.h>

#include "Auth/AuthService.h"
#include "Auth/NvsCredentialStore.h"
#include "Display/P4DetailPages.h"
#include "Display/P4OverviewView.h"
#include "Esp/EspHealthSampler.h"
#include "HttpApi/EspWebGateway.h"
#include "P4AuthTap.h"
#include "P4BuildSecrets.h"
#include "P4MeterCommands.h"
#include "P4SdHistory.h"
#include "P4StaRadio.h"
#include "Telemetry/InMemoryTelemetryStore.h"
#include "Time/NtpClock.h"
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
wattcycle::display::P4DetailPages detailPages;
wattcycle::display::P4PanelBoard board;
wattcycle::telemetry::InMemoryTelemetryStore store;
wattcycle::time_sync::NtpClock ntpClock;
wattcycle::auth::NvsCredentialStore credentials;
wattcycle::auth::AuthService authService(credentials);
P4SdHistory history;
P4MeterCommands meterCommands;
wattcycle::web::EspWebGateway webGateway(store, authService, meterCommands, history);
wattcycle::esp_sys::EspHealthSampler health;
P4AuthTap authTap;
P4StaRadio radio;
bool webNamed = false;

void onAuthTap(void* /*user*/) {
  authService.onConfirmPressed();
}

void paintBoard() {
  overview.apply(board.overview);
  overview.setLinks(board.gateway.wifiUp,
                    board.gateway.bmsUp || board.gateway.ecoUp || board.gateway.solarUp);
  detailPages.apply(board);
}

void applyHealth() {
  auto snap = health.snapshot();
  snap.coprocessorHeapKb = radio.coprocessorHeapKb();
  store.updateEspHealth(snap);
  store.setNtpSynced(ntpClock.isSynced());
  board.esp.cpu0 = snap.cpuCore0Percent;
  board.esp.cpu1 = snap.cpuCore1Percent;
  board.esp.tempC = static_cast<int>(snap.chipTemperatureC + 0.5f);
  board.esp.uptimeSec = static_cast<int>(snap.uptimeSeconds);
  board.esp.heapKb = static_cast<int>(snap.freeHeapBytes / 1024u);
}

void recordHistory() {
  if (!history.isReady() || !ntpClock.isSynced()) {
    return;
  }
  storage::HistorySample sample = {};
  sample.epochUtc = ntpClock.nowEpoch();
  const auto battery = store.battery();
  const auto solar = store.solar();
  sample.socPercent = static_cast<uint8_t>(battery.stateOfChargePercent);
  sample.packVoltageV = battery.moduleVoltage;
  sample.packCurrentA = battery.currentAmps;
  sample.packPowerW = battery.powerWatts;
  sample.packValid = battery.valid;
  sample.packRemainingAh = battery.remainingCapacityAh;
  sample.packMosTempC = battery.mosTemperatureC;
  sample.packPcbTempC = battery.pcbTemperatureC;
  sample.solarVoltageV = solar.voltageV;
  sample.solarCurrentA = solar.currentA;
  sample.solarPowerW = solar.powerW;
  sample.solarValid = solar.meterValid;
  sample.solarEnergyWh = solar.energyWh;
  sample.solarCapacityAh = solar.capacityAh;
  history.append(sample);
}

void noteWebIdentity() {
  if (webNamed || board.esp.deviceMac[0] == '\0') {
    return;
  }
  webGateway.setDeviceIdentity("p4_hub", board.esp.deviceMac, P4_BMS_MAC, P4_SOLAR_MAC, P4_ECO_MAC);
  webNamed = true;
}

constexpr uint32_t kBlankAfterMs = 60000;
uint32_t lastActivityMs = 0;
bool screenOn = true;

void setBacklight(bool on) {
  lcd.example_bsp_set_lcd_backlight(on ? 1u : 0u);
  screenOn = on;
}

void noteActivity() {
  lastActivityMs = millis();
  if (!screenOn) {
    setBacklight(true);
  }
}

void blankIfIdle() {
  if (!screenOn || lastActivityMs == 0) {
    return;
  }
  if (static_cast<int32_t>(millis() - lastActivityMs) >= static_cast<int32_t>(kBlankAfterMs)) {
    setBacklight(false);
  }
}

void onMeterCommand(uint8_t command, void* /*user*/) {
  meterCommands.sendMeterCommand(command);
}

void onPanelPage(uint8_t index, void* user) {
  auto* pages = static_cast<display::P4DetailPages*>(user);
  if (pages != nullptr) {
    pages->show(static_cast<display::HubPage>(index));
  }
}

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
  const bool wasDark = !screenOn;
  noteActivity();
  if (wasDark) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  data->state = LV_INDEV_STATE_PR;
  data->point.x = x;
  data->point.y = y;
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
  detailPages.mount(lv_scr_act());
  detailPages.setMeterCommandHandler(onMeterCommand, nullptr);
  overview.setPageHandler(onPanelPage, &detailPages);
  overview.mount(lv_scr_act(), "waiting for bridges");
  board = {};
  radio.begin(board, store);
  ntpClock.begin();
  authService.begin();
  history.begin();
  authTap.mount(lv_scr_act());
  authTap.setHandler(onAuthTap, nullptr);
  webGateway.setDeviceIdentity("p4_hub", "", P4_BMS_MAC, P4_SOLAR_MAC, P4_ECO_MAC);
  if (!webGateway.begin(6789)) {
    Serial.println("web fs failed");
  }
  store.setWebPort(6789);
  health.begin();
  paintBoard();
  lastActivityMs = millis();
  Serial.println("overview ready");
}

void noteStorage() {
  static uint32_t nextMs = 0;
  const uint32_t now = millis();
  if (nextMs != 0 && static_cast<int32_t>(now - nextMs) < 0) {
    return;
  }
  nextMs = now + 5000;
  uint32_t usedMb = 0;
  uint32_t totalMb = 0;
  const bool ready = history.storage(usedMb, totalMb);
  const bool changed = ready != board.esp.sdReady || static_cast<int>(usedMb) != board.esp.sdUsedMb ||
                       static_cast<int>(totalMb) != board.esp.sdTotalMb;
  board.esp.sdReady = ready;
  board.esp.sdUsedMb = static_cast<int>(usedMb);
  board.esp.sdTotalMb = static_cast<int>(totalMb);
  overview.setStorage(ready, board.esp.sdUsedMb, board.esp.sdTotalMb);
  if (changed) {
    paintBoard();
  }
}

void loopPanel() {
  const bool dirty = radio.loop(board);
  ntpClock.loop(board.gateway.wifiUp);
  if (dirty) {
    paintBoard();
  }
  if (health.sample()) {
    applyHealth();
    paintBoard();
  }
  recordHistory();
  authTap.apply(authService.prompt().kind);
  noteWebIdentity();
  webGateway.loop();
  noteStorage();
  blankIfIdle();
  overview.refreshClock();
  lv_timer_handler();
  delay(5);
}

}  // namespace wattcycle::panel
