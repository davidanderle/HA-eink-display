#include "battery.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

// The UM ProS3 senses the LiPo voltage on IO10 through a 1/2 divider
#define BATTERY_SENSE_GPIO (10)
#define BATTERY_DIVIDER    (2)
#define BATTERY_SAMPLES    (16)

static adc_oneshot_unit_handle_t adc;
static adc_cali_handle_t cali;
static adc_channel_t channel;

// Typical LiPo discharge curve (mV -> %) at a low load
static const struct { uint16_t mv; uint8_t pct; } curve[] = {
    {3300, 0}, {3500, 5}, {3600, 15}, {3700, 35}, {3800, 55},
    {3900, 70}, {4000, 80}, {4100, 90}, {4200, 100},
};

void battery_init(void) {
    adc_unit_t unit;
    if(adc_oneshot_io_to_channel(BATTERY_SENSE_GPIO, &unit, &channel) != ESP_OK ||
       adc_oneshot_new_unit(&(adc_oneshot_unit_init_cfg_t){ .unit_id = unit }, &adc) != ESP_OK ||
       adc_oneshot_config_channel(adc, channel, &(adc_oneshot_chan_cfg_t){
           .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT }) != ESP_OK ||
       adc_cali_create_scheme_curve_fitting(&(adc_cali_curve_fitting_config_t){
           .unit_id = unit, .chan = channel, .atten = ADC_ATTEN_DB_12,
           .bitwidth = ADC_BITWIDTH_DEFAULT }, &cali) != ESP_OK) {
        ESP_LOGE("BATT", "Failed to initialise the battery ADC");
        adc = NULL;
    }
}

uint8_t battery_get_percent(void) {
    if(adc == NULL) return 100;

    int32_t sum_mv = 0;
    int32_t n = 0;
    for(int i = 0; i < BATTERY_SAMPLES; i++) {
        int raw, mv;
        if(adc_oneshot_read(adc, channel, &raw) == ESP_OK && adc_cali_raw_to_voltage(cali, raw, &mv) == ESP_OK) {
            sum_mv += mv;
            n++;
        }
    }
    if(n == 0) return 100;

    const uint32_t vbat = (uint32_t)(sum_mv / n) * BATTERY_DIVIDER;
    if(vbat <= curve[0].mv) return 0;
    for(size_t i = 1; i < sizeof(curve)/sizeof(curve[0]); i++) {
        if(vbat <= curve[i].mv) {
            const uint32_t span = curve[i].mv - curve[i-1].mv;
            return curve[i-1].pct + (curve[i].pct - curve[i-1].pct) * (vbat - curve[i-1].mv) / span;
        }
    }
    return 100;
}
