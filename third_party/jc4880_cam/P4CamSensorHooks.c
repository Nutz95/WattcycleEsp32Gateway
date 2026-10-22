#include "esp_cam_sensor_detect.h"
#include "esp_ipa.h"
#include "ov02c10.h"

#include <stdlib.h>
#include <string.h>

extern void __real_esp_cam_sensor_detect_get_array(esp_cam_sensor_detect_fn_t **array_start_ptr,
                                                   esp_cam_sensor_detect_fn_t **array_end_ptr);
extern const esp_ipa_config_t *__real_esp_ipa_pipeline_get_config(const char *name);
extern const esp_ipa_config_t *p4_ov02c10_ipa_get_config(const char *name);
extern esp_cam_sensor_device_t *__esp_cam_sensor_detect_fn_ov02c10_detect_ESP_CAM_SENSOR_MIPI_CSI(void *config);

static esp_cam_sensor_detect_fn_t *s_detect_table = NULL;
static size_t s_detect_count = 0;

void __wrap_esp_cam_sensor_detect_get_array(esp_cam_sensor_detect_fn_t **array_start_ptr,
                                            esp_cam_sensor_detect_fn_t **array_end_ptr) {
  if (s_detect_table == NULL) {
    esp_cam_sensor_detect_fn_t *start = NULL;
    esp_cam_sensor_detect_fn_t *end = NULL;
    __real_esp_cam_sensor_detect_get_array(&start, &end);
    const size_t stock = (start != NULL && end != NULL && end > start) ? (size_t)(end - start) : 0;
    s_detect_count = stock + 1;
    s_detect_table = calloc(s_detect_count, sizeof(esp_cam_sensor_detect_fn_t));
    if (s_detect_table == NULL) {
      __real_esp_cam_sensor_detect_get_array(array_start_ptr, array_end_ptr);
      return;
    }
    s_detect_table[0].detect = __esp_cam_sensor_detect_fn_ov02c10_detect_ESP_CAM_SENSOR_MIPI_CSI;
    s_detect_table[0].port = ESP_CAM_SENSOR_MIPI_CSI;
    s_detect_table[0].sccb_addr = OV02C10_SCCB_ADDR;
    if (stock > 0) {
      memcpy(s_detect_table + 1, start, stock * sizeof(esp_cam_sensor_detect_fn_t));
    }
  }
  *array_start_ptr = s_detect_table;
  *array_end_ptr = s_detect_table + s_detect_count;
}

const esp_ipa_config_t *__wrap_esp_ipa_pipeline_get_config(const char *name) {
  const esp_ipa_config_t *local = p4_ov02c10_ipa_get_config(name);
  if (local != NULL) {
    return local;
  }
  return __real_esp_ipa_pipeline_get_config(name);
}
