#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "unity.h"
#include "it8951.h"
#include "test_it8951.h"

static const char *tag = "TEST";

// Expose the low-level testee functions from the driver
extern bool send_command(stIT8951_Handler_t *hdlr, const eIT8951_Command_t cmd);
extern bool write_bytes(stIT8951_Handler_t *hdlr, const uint8_t *const data, const int32_t count);
extern bool read_data(stIT8951_Handler_t *hdlr, uint16_t *const data, const int32_t count);
extern bool write_data(stIT8951_Handler_t *hdlr, const uint16_t *const data, const int32_t count);

/// @brief Used to store the current state of the nCS pin
static bool ncs = 1;
static uint8_t *_txdata;
static uint16_t *_rxdata;
static uint32_t _txcount = 0;
static uint32_t _cnt = 0;

// TODO: These tests should be rewritten with parametric testing (see Claude) 

static bool mock_spi_transcieve(const void *tx, void *rx, size_t len) {
    // This big-endian buffer is simulateing data returned by the IT8951. Note:
    // the first 4 bytes are dummy and should be ignored by the processing func
    static const uint8_t rxdata[] = {0x00,0x00,0x00,0x00,0x12,0x34,0x00,0x00,0x7F,0xFF,0x80,0x00,0xFF,0xFF};
    assert(len <= sizeof(rxdata));

    _cnt++;

    if(!_txdata) {
        ESP_LOGE(tag, "txdata buffer cannot be NULL");
        return false;
    } else {
        TEST_ASSERT_FALSE(ncs);
        memcpy(&_txdata[_txcount], tx, len);
        if(rx){
            memcpy(rx, &rxdata[_txcount], len);
        }
        _txcount += len;
        for(uint32_t i=0; i<len; i++){
            if(rx) {
                ESP_LOGI(tag, "tx: 0x%02x, rx: 0x%02x", ((uint8_t*)tx)[i], ((uint8_t*)rx)[i]);
            } else {
                ESP_LOGI(tag, "tx: 0x%02x", ((uint8_t*)tx)[i]);
            }
        }
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

void test_send_command(const eIT8951_Command_t cmd, uint8_t *expected_tx) {
    const size_t size = (1+1)*sizeof(uint16_t);
    _txdata = calloc(size, sizeof(*_txdata));
    _txcount = _cnt =  0;
    if(_txdata) {
        TEST_ASSERT_TRUE(ncs);
        TEST_ASSERT_TRUE(send_command(&hdlr, cmd));
        TEST_ASSERT_TRUE(ncs);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_tx, _txdata, size);
        TEST_ASSERT_EQUAL(size/2, _cnt);
        free(_txdata);
    } else {
        printf("TEST: Insufficient memory!\n");
    }
}
void test_send_command_multiple_args(void) {
    test_send_command(IT8951_COMMAND_SYS_RUN,      (uint8_t[]){0x60, 0x00, 0x00, 0x01});
    test_send_command(IT8951_COMMAND_STANDBY,      (uint8_t[]){0x60, 0x00, 0x00, 0x02});
    test_send_command(IT8951_COMMAND_CMD_VCOM,     (uint8_t[]){0x60, 0x00, 0x00, 0x39});
    test_send_command(IT8951_COMMAND_GET_DEV_INFO, (uint8_t[]){0x60, 0x00, 0x03, 0x02});
}

void test_write_data(const uint16_t *const data, int32_t count, uint8_t *expected_tx) {
    const size_t size = (1+count)*sizeof(uint16_t);
    _txdata = calloc(size, sizeof(*_txdata));
    _txcount = _cnt =  0;
    if(_txdata) {
        TEST_ASSERT_TRUE(ncs);
        TEST_ASSERT_TRUE(write_data(&hdlr, data, count));
        TEST_ASSERT_TRUE(ncs);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_tx, _txdata, size);
        TEST_ASSERT_EQUAL(size/2, _cnt);
        free(_txdata);
    } else {
        ESP_LOGE(tag, "Insufficient memory!");
    }
}
void test_write_data_multiple_args(void) {
    test_write_data((uint16_t[]){0x0000, 0x0203, 0x8567}, 3, 
                    (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x85, 0x67});
    test_write_data((uint16_t[]){0x7FFF}, 1,
                    (uint8_t []){0x00, 0x00, 0x7F, 0xFF});
    test_write_data((uint16_t[]){0xFFFF}, 1,
                    (uint8_t []){0x00, 0x00, 0xFF, 0xFF});
    test_write_data((uint16_t[]){}, 0, (uint8_t[]){});
}

void test_read_data(const int32_t count, uint8_t *expected_tx, uint16_t *expected_rx) {
    const size_t size = (2+count)*sizeof(uint16_t);
    _txdata = calloc(size, sizeof(*_txdata));
    _rxdata = calloc(count, sizeof(*_rxdata));
    _txcount = _cnt =  0;
    if(_txdata && (_rxdata || count == 0)) {
        TEST_ASSERT_TRUE(ncs);
        TEST_ASSERT_TRUE(read_data(&hdlr, _rxdata, count));
        TEST_ASSERT_TRUE(ncs);
        TEST_ASSERT_EQUAL(size/2, _cnt);
        if(count != 0){
            TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_tx, _txdata, size);
            TEST_ASSERT_EQUAL_HEX16_ARRAY(expected_rx, _rxdata, count);
        }
        free(_txdata);
        free(_rxdata);
    } else {
        ESP_LOGE(tag, "Insufficient memory!");
    }
}
void test_read_data_multiple_args(void) {
    test_read_data(0, (uint8_t[]){}, 
                      (uint16_t[]){});
    test_read_data(1, (uint8_t[]){0x10,0x00,0x00,0x00,0x00,0x00}, 
                      (uint16_t[]){0x1234});
    test_read_data(5, (uint8_t[]){0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, 
                      (uint16_t[]){0x1234, 0x0000, 0x7FFF, 0x8000, 0xFFFF});
}

void test_pack_pixels(eIT8951_ColorDepth_t bpp, stRectangle_t *rect, uint16_t *expected_out, uint32_t count){
    const uint32_t area = rectangle_get_area(rect);
    uint8_t *in_pixels = malloc(area*sizeof(*in_pixels));
    memset(in_pixels, 0x0F, area);
    // TODO: 4 is 16bit/4bpp ->4pix/word
    uint16_t *packed_pixels = malloc((rect->width/4+2)*rect->height*sizeof(uint16_t));

    if(packed_pixels && in_pixels) {
        uint32_t cnt;
        it8951_pack_pixels(&(stIT8951_ImageInfo_t){
            .endianness = IT8951_ENDIANNESS_LITTLE,
            .bpp = bpp,
            .rotation = IT8951_ROTATION_MODE_0
        }, rect, in_pixels, packed_pixels, &cnt);
        TEST_ASSERT_EQUAL_UINT32(count, cnt);
        TEST_ASSERT_EQUAL_HEX16_ARRAY(expected_out, packed_pixels, count);
        if(count == 1)
            ESP_LOGI(tag, "%x", packed_pixels[0]);
        else if(count == 2)
            ESP_LOGI(tag, "%x,%x", packed_pixels[0], packed_pixels[1]);
        else if(count == 4)
            ESP_LOGI(tag, "%x,%x,%x,%x", packed_pixels[0], packed_pixels[1],packed_pixels[2], packed_pixels[3]);
    } else {
        ESP_LOGE(tag, "Insufficient memory!");
    }
    free(in_pixels);
    free(packed_pixels);
}

void test_pack_pixels_multiple_args(void) {
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){0, 0, 4, 1}, (uint16_t[]){0xFFFF}, 1),
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){3, 0, 4, 1}, (uint16_t[]){0xF000, 0x0FFF}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){0, 0, 1, 4}, (uint16_t[]){0x000F,0x000F,0x000F,0x000F}, 4),
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){0, 0, 2, 2}, (uint16_t[]){0x00FF, 0x00FF}, 2),
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){0, 1, 2, 2}, (uint16_t[]){0x00FF, 0x00FF}, 2),
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){0, 2, 2, 2}, (uint16_t[]){0x00FF, 0x00FF}, 2),
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){0, 3, 2, 2}, (uint16_t[]){0x00FF, 0x00FF}, 2),
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){1, 0, 2, 2}, (uint16_t[]){0x0FF0, 0x0FF0}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){1, 1, 2, 2}, (uint16_t[]){0x0FF0, 0x0FF0}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){1, 2, 2, 2}, (uint16_t[]){0x0FF0, 0x0FF0}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){1, 3, 2, 2}, (uint16_t[]){0x0FF0, 0x0FF0}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){2, 0, 2, 2}, (uint16_t[]){0xFF00, 0xFF00}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){2, 1, 2, 2}, (uint16_t[]){0xFF00, 0xFF00}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){2, 2, 2, 2}, (uint16_t[]){0xFF00, 0xFF00}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){2, 3, 2, 2}, (uint16_t[]){0xFF00, 0xFF00}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){3, 0, 2, 2}, (uint16_t[]){0xF000, 0x000F, 0xF000, 0x000F}, 4);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){3, 1, 2, 2}, (uint16_t[]){0xF000, 0x000F, 0xF000, 0x000F}, 4);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){3, 2, 2, 2}, (uint16_t[]){0xF000, 0x000F, 0xF000, 0x000F}, 4);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){3, 3, 2, 2}, (uint16_t[]){0xF000, 0x000F, 0xF000, 0x000F}, 4);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){4, 0, 2, 2}, (uint16_t[]){0x00FF, 0x00FF}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){4, 1, 2, 2}, (uint16_t[]){0x00FF, 0x00FF}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){4, 2, 2, 2}, (uint16_t[]){0x00FF, 0x00FF}, 2);
    test_pack_pixels(IT8951_COLOR_DEPTH_BPP_4BIT, &(stRectangle_t){4, 3, 2, 2}, (uint16_t[]){0x00FF, 0x00FF}, 2);
}

// This is required for the ESP-IDF framework
void app_main() {
    UNITY_BEGIN();

    RUN_TEST(test_send_command_multiple_args);
    RUN_TEST(test_write_data_multiple_args);
    RUN_TEST(test_read_data_multiple_args);
    //RUN_TEST(test_pack_pixels_multiple_args);

    UNITY_END();
}
