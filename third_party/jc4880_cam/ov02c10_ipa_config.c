/*
* SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: ESPRESSIF MIT
*/

#include <string.h>
#include "esp_ipa.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

typedef struct esp_video_ipa_index {
    const char *name;
    const char *description;
    const esp_ipa_config_t *ipa_config;
} esp_video_ipa_index_t;

static const esp_ipa_awb_config_t s_ipa_awb_OV02C10_0_config = {
    .model = ESP_IPA_AWB_MODEL_0,
    .min_counted = 2000,
    .min_red_gain_step = 0.34,
    .min_blue_gain_step = 0.4,
    .red_gain_scale = 1.000000f,
    .blue_gain_scale = 1.000000f,
    .range = {
        .green_max = 190,
        .green_min = 81,
        .rg_max = 0.9096,
        .rg_min = 0.573,
        .bg_max = 0.9634,
        .bg_min = 0.5368
    },
    .green_luma_env = "dummy_awb_luma",
    .green_luma_init = 91,
    .green_luma_step_ratio = 0.3,
    .enable_sub_win = false,
    .min_subwin_wp_counted = 0,
    .min_subwin_participated = 0,
    .subwin_weight = {
        { 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f },
        { 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f },
        { 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f },
        { 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f },
        { 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f },
    },
    .subwin_green_dark = 0,
    .subwin_green_mid = 100,
    .subwin_green_bright = 200,
    .zones = NULL,
    .zones_count = 0,
    .ref_points = NULL,
    .ref_points_count = 0,
    .new_w = 0.300000f,
    .prev_w = 0.700000f,
    .export_ct = false,
    .outlier_rg = 0.000000f,
    .outlier_bg = 0.000000f,
    .zone_hysteresis_ratio = 0.000000f,
    .zone_switch_count = 0,
    .type_counter_max = 20000};

static const esp_ipa_agc_meter_light_threshold_t s_ipa_agc_meter_light_thresholds_OV02C10_0[] = {
    {
        .luma_threshold = 20,
        .weight_offset = 1,
    },
    {
        .luma_threshold = 55,
        .weight_offset = 2,
    },
    {
        .luma_threshold = 95,
        .weight_offset = 3,
    },
    {
        .luma_threshold = 155,
        .weight_offset = 4,
    },
    {
        .luma_threshold = 235,
        .weight_offset = 5,
    },
};

static const esp_ipa_agc_config_t s_ipa_agc_OV02C10_0_config = {
    .exposure_frame_delay = 3,
    .exposure_adjust_delay = 0,
    .gain_frame_delay = 3,
    .min_gain_step = 0.03,
    .max_gain = 0,
    .inc_gain_ratio = 0.32,
    .dec_gain_ratio = 0.42,
    .anti_flicker_mode = ESP_IPA_AGC_ANTI_FLICKER_FULL,
    .ac_freq = 50,
    .gain_only = false,
    .fixed_exposure_time = 0,
    .luma_low = 48,
    .luma_high = 85,
    .luma_target = 75,
    .luma_low_threshold = 14,
    .luma_low_regions = 5,
    .luma_high_threshold = 239,
    .luma_high_regions = 3,
    .luma_weight_table = {
        1, 1, 2, 1, 1, 1, 2, 3, 2, 1, 1, 3, 5, 3, 1, 1, 2, 3, 2, 1, 1, 1, 2, 1, 1, 
    },
    .meter_mode = ESP_IPA_AGC_METER_HIGHLIGHT_PRIOR,
    .high_light_prior_config = {
        .use_env_luma = false,
        .luma_high_threshold = 202,
        .luma_low_threshold = 119,
        .weight_offset = 5,
        .luma_offset = -1
    },
    .low_light_prior_config = {
        .use_env_luma = false,
        .luma_high_threshold = 64,
        .luma_low_threshold = 49,
        .weight_offset = 5,
        .luma_offset = 1
    },
    .light_threshold_config = {
        .use_env_luma = false,
        .table = s_ipa_agc_meter_light_thresholds_OV02C10_0,
        .table_size = ARRAY_SIZE(s_ipa_agc_meter_light_thresholds_OV02C10_0)
    },
};

static const esp_ipa_ian_luma_ae_config_t s_esp_ipa_ian_luma_ae_OV02C10_0_config = {                 
    .weight = {
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    },
};

static const esp_ipa_ian_luma_config_t s_esp_ipa_ian_luma_OV02C10_0_config = {
    .ae = &s_esp_ipa_ian_luma_ae_OV02C10_0_config,
};

static const esp_ipa_ian_config_t s_ipa_ian_OV02C10_0_config = {
    .luma = &s_esp_ipa_ian_luma_OV02C10_0_config,
};

static const esp_ipa_acc_sat_t s_ipa_acc_sat_OV02C10_0_config[] = {
    {
        .color_temp = 0,
        .saturation = 136
    },
};

static const esp_ipa_acc_ccm_unit_t s_esp_ipa_acc_ccm_OV02C10_0_table[] = {
    {
        .color_temp = 0,
        .ccm = {
            .matrix = {
                { 1.408, -0.094, -0.314 },
                { -0.13, 1.28, -0.15 },
                { -0.072, -0.173, 1.245 }
            }
        }
    },
};

static const esp_ipa_acc_ccm_config_t s_esp_ipa_acc_ccm_OV02C10_0_config = {
    .model = 0,
    .luma_env = "ae.luma.avg",
    .luma_low_threshold = 28,
    .luma_low_ccm = {
        .matrix = {
            { 1.0, 0.0, 0.0 },
            { 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 1.0 }
        }
    }
    ,
    .ccm_table = s_esp_ipa_acc_ccm_OV02C10_0_table,
    .ccm_table_size = 1,
};

static const esp_ipa_acc_config_t s_ipa_acc_OV02C10_0_config = {
    .sat_table = s_ipa_acc_sat_OV02C10_0_config,
    .sat_table_size = ARRAY_SIZE(s_ipa_acc_sat_OV02C10_0_config),
    .ccm = &s_esp_ipa_acc_ccm_OV02C10_0_config,
};

static const esp_ipa_adn_bf_t s_ipa_adn_bf_OV02C10_0_config[] = {
    {
        .gain = 1000,
        .bf = {
            .level = 3,
            .matrix = {
                {2, 4, 2},
                {4, 5, 4},
                {2, 4, 2}
            }
        }
    },
    {
        .gain = 4000,
        .bf = {
            .level = 3,
            .matrix = {
                {1, 3, 1},
                {3, 5, 3},
                {1, 3, 1}
            }
        }
    },
    {
        .gain = 8000,
        .bf = {
            .level = 4,
            .matrix = {
                {1, 3, 1},
                {3, 4, 3},
                {1, 3, 1}
            }
        }
    },
    {
        .gain = 16000,
        .bf = {
            .level = 5,
            .matrix = {
                {1, 3, 1},
                {3, 5, 3},
                {1, 3, 1}
            }
        }
    },
    {
        .gain = 24000,
        .bf = {
            .level = 6,
            .matrix = {
                {1, 2, 1},
                {2, 3, 2},
                {1, 2, 1}
            }
        }
    },
    {
        .gain = 32000,
        .bf = {
            .level = 7,
            .matrix = {
                {1, 2, 1},
                {2, 4, 2},
                {1, 2, 1}
            }
        }
    },
    {
        .gain = 64000,
        .bf = {
            .level = 7,
            .matrix = {
                {1, 2, 1},
                {2, 2, 2},
                {1, 2, 1}
            }
        }
    },
};

static const esp_ipa_adn_dm_t s_ipa_adn_dm_OV02C10_0_config[] = {
    {
        .gain = 1000,
        .dm = {
            .gradient_ratio = 1.5
        }
    },
    {
        .gain = 4000,
        .dm = {
            .gradient_ratio = 1.25
        }
    },
    {
        .gain = 8000,
        .dm = {
            .gradient_ratio = 1.05
        }
    },
    {
        .gain = 12000,
        .dm = {
            .gradient_ratio = 1.0
        }
    },
};

static const esp_ipa_adn_config_t s_ipa_adn_OV02C10_0_config = {
    .bf_table = s_ipa_adn_bf_OV02C10_0_config,
    .bf_table_size = ARRAY_SIZE(s_ipa_adn_bf_OV02C10_0_config),
    .dm_table = s_ipa_adn_dm_OV02C10_0_config,
    .dm_table_size = ARRAY_SIZE(s_ipa_adn_dm_OV02C10_0_config),
};

static const esp_ipa_aen_gamma_unit_t s_esp_ipa_aen_gamma_OV02C10_0_table[] = {
    {
        .luma = 71.1,
        .gamma = {
            .red = {
                .x = { 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,  },
                .y = { 63, 90, 111, 129, 144, 159, 172, 184, 196, 207, 217, 227, 237, 246, 255, 255,  }
            },
            .green = {
                .x = { 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,  },
                .y = { 63, 90, 111, 129, 144, 159, 172, 184, 196, 207, 217, 227, 237, 246, 255, 255,  }
            },
            .blue = {
                .x = { 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,  },
                .y = { 63, 90, 111, 129, 144, 159, 172, 184, 196, 207, 217, 227, 237, 246, 255, 255,  }
            }
        }
    },
};

static const esp_ipa_aen_gamma_config_t s_ipa_aen_gamma_OV02C10_0_config = {
    .model = 0,
    .luma_env = "ae.luma.avg",
    .luma_min_step = 16.0,
    .gamma_table = s_esp_ipa_aen_gamma_OV02C10_0_table,
    .gamma_table_size = 1,
};

static const esp_ipa_aen_sharpen_t s_ipa_aen_sharpen_OV02C10_0_config[] = {
    {
        .gain = 1000,
        .sharpen = {
            .h_thresh = 25,
            .l_thresh = 5,
            .h_coeff = 1.925,
            .m_coeff = 1.825,
            .matrix = {
                {1, 2, 1},
                {2, 2, 2},
                {1, 2, 1}
            }
        }
    },
    {
        .gain = 8000,
        .sharpen = {
            .h_thresh = 20,
            .l_thresh = 5,
            .h_coeff = 1.825,
            .m_coeff = 1.425,
            .matrix = {
                {2, 2, 2},
                {2, 1, 2},
                {2, 2, 2}
            }
        }
    },
    {
        .gain = 12000,
        .sharpen = {
            .h_thresh = 16,
            .l_thresh = 6,
            .h_coeff = 1.625,
            .m_coeff = 1.325,
            .matrix = {
                {1, 1, 1},
                {1, 1, 1},
                {1, 1, 1}
            }
        }
    },
    {
        .gain = 65000,
        .sharpen = {
            .h_thresh = 20,
            .l_thresh = 5,
            .h_coeff = 1.625,
            .m_coeff = 1.225,
            .matrix = {
                {1, 1, 1},
                {1, 1, 1},
                {1, 1, 1}
            }
        }
    },
};

static const esp_ipa_aen_con_t s_ipa_aen_con_OV02C10_0_config[] = {
    {
        .gain = 1000,
        .contrast = 132
    },
    {
        .gain = 16000,
        .contrast = 130
    },
    {
        .gain = 24000,
        .contrast = 128
    },
    {
        .gain = 65000,
        .contrast = 126
    },
};

static const esp_ipa_aen_config_t s_ipa_aen_OV02C10_0_config = {
    .gamma = &s_ipa_aen_gamma_OV02C10_0_config,
    .sharpen_table = s_ipa_aen_sharpen_OV02C10_0_config,
    .sharpen_table_size = ARRAY_SIZE(s_ipa_aen_sharpen_OV02C10_0_config),
    .con_table = s_ipa_aen_con_OV02C10_0_config,
    .con_table_size = ARRAY_SIZE(s_ipa_aen_con_OV02C10_0_config),
};

static const char *s_ipa_OV02C10_0_names[] = {
    "esp_ipa_awb",
    "esp_ipa_agc",
    "esp_ipa_ian",
    "esp_ipa_acc",
    "esp_ipa_adn",
    "esp_ipa_aen",
};

static const esp_ipa_config_t s_ipa_OV02C10_0_config = {
    .names = s_ipa_OV02C10_0_names,
    .nums = ARRAY_SIZE(s_ipa_OV02C10_0_names),
    .version = 1,
    .awb = &s_ipa_awb_OV02C10_0_config,
    .agc = &s_ipa_agc_OV02C10_0_config,
    .ian = &s_ipa_ian_OV02C10_0_config,
    .acc = &s_ipa_acc_OV02C10_0_config,
    .adn = &s_ipa_adn_OV02C10_0_config,
    .aen = &s_ipa_aen_OV02C10_0_config,
};

static const esp_video_ipa_index_t s_video_ipa_configs[] = {
    {
        .name = "OV02C10",
        .ipa_config = &s_ipa_OV02C10_0_config
    },
};

const esp_ipa_config_t *p4_ov02c10_ipa_get_config(const char *name)
{
    for (int i = 0; i < ARRAY_SIZE(s_video_ipa_configs); i++) {
        if (!strcmp(name, s_video_ipa_configs[i].name)) {
            return s_video_ipa_configs[i].ipa_config;
        }
    }
    return NULL;
}
const esp_ipa_config_t *p4_ov02c10_ipa_enum_configs(const char *sensor_name, int index)
{
    int n = 0;
    if (!sensor_name || index < 0) {
        return NULL;
    }
    for (int i = 0; i < ARRAY_SIZE(s_video_ipa_configs); i++) {
        if (!strcmp(sensor_name, s_video_ipa_configs[i].name)) {
            if (n == index) {
                return s_video_ipa_configs[i].ipa_config;
            }
            n++;
        }
    }
    return NULL;
}

/* Json file: ov02c10_default_p4_eco4.json */

