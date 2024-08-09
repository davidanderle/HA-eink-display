#include "it8951.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static stIT8951_Handler_t it8951_hdlr;
static spi_device_handle_t spi = {};

static gpio_num_t ncs = 34;
static gpio_num_t hrdy = 9;

static inline bool it8951_transcieve(const void *txdata, void *rxdata, size_t len) {
    return spi_device_transmit(spi, &(spi_transaction_t) {
        .length = len*8,
        .tx_buffer = txdata,
        .rx_buffer = rxdata
    }) == ESP_OK;
}

static inline void it8951_set_ncs(bool state) {
    gpio_set_level(ncs, state);
}

static inline bool it8951_get_hrdy(void) {
    return gpio_get_level(hrdy);
}

void display_init(void) {
    it8951_hdlr = (stIT8951_Handler_t) {
        .spi_transcieve = it8951_transcieve,
        .set_ncs        = it8951_set_ncs,
        .get_hrdy       = it8951_get_hrdy,
        .vcom_mv        = INT_MAX,
        
    };
    it8951_init(&it8951_hdlr);
}