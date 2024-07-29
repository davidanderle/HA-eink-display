#include "esp_sleep.h"
#include "esp_log.h"

#define WAKEUP_PERIOD_US (30*60*1000000)

void Hibernate(void){
    // Configure the RTC to wake up the device every T minutes
    esp_sleep_enable_timer_wakeup(WAKEUP_PERIOD_US);
    ESP_LOGI("Hibernate", "Entering hibernation for 30 minutes");
    // Enter hibernation
    esp_deep_sleep_start();
}
