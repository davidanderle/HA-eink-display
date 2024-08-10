#include "it8951.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static stIT8951_Handler_t it8951_hdlr;
static spi_device_handle_t spi;

static const gpio_num_t ncs = 34;
static const gpio_num_t hrdy = 9;
static const gpio_num_t spi_mosi = 35;
static const gpio_num_t spi_miso = 37;
static const gpio_num_t spi_clk = 36;

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
    ESP_ERROR_CHECK(spi_bus_initialize(SPI1_HOST, &(spi_bus_config_t){
        .miso_io_num = spi_miso,
        .mosi_io_num = spi_mosi,
        .sclk_io_num = spi_clk,
        .quadwp_io_num = -1, // Unused
        .quadhd_io_num = -1, // Unused
        // TODO: May need adjusting
        .max_transfer_sz = 4096,
    }, SPI_DMA_CH_AUTO));

    ESP_ERROR_CHECK(spi_bus_add_device(SPI1_HOST, &(spi_device_interface_config_t){
        .clock_speed_hz = 24000000,
        .mode = 0,
        .spics_io_num = -1, // Controlled externally
        .queue_size = 7,
        .pre_cb = NULL,
        .post_cb = NULL,
        .flags = 0,
        .address_bits = 0,
        .command_bits = 0,
        .duty_cycle_pos = 0,
    }, &spi));

    it8951_hdlr = (stIT8951_Handler_t) {
        .spi_transcieve = it8951_transcieve,
        .set_ncs        = it8951_set_ncs,
        .get_hrdy       = it8951_get_hrdy,
        .vcom_mv        = INT_MAX,
    };
    it8951_init(&it8951_hdlr);
}