#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "unity.h"
#include "unity_fixture.h"
#include "it8951.h"
#include "test_it8951.h"

// TODO: Need to define UNIT_TEST in the test build!

// Expose the low-level testee functions from the driver
// TODO: Changed last argument to int32_t!
extern bool send_with_preamble(stIT8951_Handler_t *hdlr, const eIT8951_SpiPreamble_t preamble, const uint16_t *const data, const int32_t count);
extern bool write_bytes(stIT8951_Handler_t *hdlr, const uint8_t *const data, const uint32_t count);
extern bool read_data(stIT8951_Handler_t *hdlr, uint16_t *const data, const uint32_t count);

/// @brief Used to store the current state of the nCS pin
static bool ncs = 1;
static uint8_t *_txdata;
static int _txcount = 0;

static bool mock_spi_transcieve(const void *tx, void *rx, size_t len) {
    if(!_txdata) {
        printf("TEST: rxdata buffer cannot be NULL\n");
        return false;
    } else {
        memcpy(&_txdata[_txcount], tx, len);
        _txcount += len;
        return true;
    }
}

static inline void mock_set_ncs(bool state) {
    ncs = state;
}

static inline bool mock_get_hrdy(void) {
    return true;
}

static stIT8951_Handler_t hdlr = {
    .spi_transcieve = mock_spi_transcieve,
    .set_ncs = mock_set_ncs,
    .vcom_mv = INT_MAX,
    .get_hrdy = mock_get_hrdy,
};

void setUp(void) {}
void tearDown(void) {}
void suiteSetUp(void) {
    printf("==[ Testing IT8951 driver... ]==\n");
}

void test_send_with_preamble(uint16_t preamb, uint16_t data, uint8_t *expected_tx) {
    _txdata = calloc(sizeof(preamb)+sizeof(data), sizeof(uint8_t));
    _txcount = 0;
    if(_txdata) {
        TEST_ASSERT_TRUE(ncs);
        TEST_ASSERT_TRUE(send_with_preamble(&hdlr, preamb, &data, 1));
        TEST_ASSERT_TRUE(ncs);
        ESP_LOGI("TEST", "TX'd [0x%02x 0x%02x 0x%02x 0x%02x]\n", 
                 _txdata[0], _txdata[1], _txdata[2], _txdata[3]);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_tx, _txdata, sizeof(preamb)+sizeof(data));
        free(_txdata);
    } else {
        printf("TEST: Insufficient memory!\n");
    }
}

void test_send_with_preamble_multiple_args(void) {
    test_send_with_preamble(IT8951_SPI_PREAMBLE_COMMAND, IT8951_COMMAND_SYS_RUN,
                           (uint8_t[]){0x60, 0x00, 0x00, 0x01});
    test_send_with_preamble(IT8951_SPI_PREAMBLE_COMMAND, IT8951_COMMAND_STANDBY,
                           (uint8_t[]){0x60, 0x00, 0x00, 0x02});
    test_send_with_preamble(IT8951_SPI_PREAMBLE_COMMAND, IT8951_COMMAND_CMD_VCOM,
                           (uint8_t[]){0x60, 0x00, 0x00, 0x39});
    test_send_with_preamble(IT8951_SPI_PREAMBLE_COMMAND, IT8951_COMMAND_GET_DEV_INFO,
                           (uint8_t[]){0x60, 0x00, 0x03, 0x02});
}

// This is required for the ESP-IDF framework
void app_main() {
    UNITY_BEGIN();

    RUN_TEST(test_send_with_preamble_multiple_args);

    UNITY_END();
}
