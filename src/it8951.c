#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "it8951.h"

static inline u32 it8951_get_pixel_per_byte(eIT8951_ColorDepth_t bpp) {
    assert(IsEnum_IT8951_ColorDepth(bpp));
    return IT8951_BPP_PER_BYTE_MAP[bpp];
}

static inline u32 it8951_bpp_to_code(const u32 bpp) {
    assert(IS_WITHIN_RANGE(bpp, 1, 4) || bpp == 8);
    return IT8951_BPP_CODE_MAP[bpp];
}

/// @brief Converts the device info struct into a string.
/// @param dev_info Pointer to the raw data encoding the device info
/// @param buff Pointer to the buffer to store the string in. The caller must
/// ensure that there's at least 158 bytes of memory allocated in this buffer.
/// @return Pointer to the beginning of the string buffer
char *it8951_device_info_to_string(const stIT8951_DeviceInfo_t *const dev_info, char *buff) {
    sprintf(buff, "Panel width: %u\n"
                  "Panel height: %u\n"
                  "Image buffer address: 0x%lx\n"
                  "Firmware version: %s\n"
                  "LuT version: %s", 
                  dev_info->panel_width, 
                  dev_info->panel_height,
                  dev_info->img_buff_addr, 
                  dev_info->firmware_version, 
                  dev_info->lut_version);
    return buff;
}

/// @brief Calculates the area of the rectangle
/// @param rect Pointer to the rectangle
/// @return Area of the rectangle
__attribute__((const))
u32 rectangle_get_area(const stRectangle *const rect){
    return rect->width * rect->height;
}

/// @brief Checks if 2 rectangles overlap
/// @param rect1 Pointer to the first rectangle
/// @param rect2 Pointer to the second rectangle
/// @return True if the rectangles overlap, false otherwise
bool rectangle_is_contained_within(const stRectangle *const rect1, const stRectangle *const rect2) {
    return (rect1->x >= rect2->x &&
            rect1->y >= rect2->y &&
            rect1->x + rect1->width  <= rect2->x + rect2->width &&
            rect1->y + rect1->height <= rect2->x + rect2->height);
}

/// @brief Converts a rectangle into a string
/// @param rect Rectangle to stringify
/// @param buff Buffer to store the string in. Must be at least 53 bytes.
/// @return Pointer to the beginning of the buffer
char *rectangle_to_string(const stRectangle *const rect, char *buff) {
    sprintf(buff, "(x,y): (%u,%u)\n"
                  "(w,h): (%u,%u)\n"
                  "Area: %lu", 
                  rect->x, rect->y, rect->width, rect->height, 
                  rectangle_get_area(rect));
    return buff;
}

/// @brief The host must wait for the HRDY pin to be high before Tx/Rx of the 
/// next 2x8 bits.
/// @param hdlr Pointer to the IT8951 handler
static inline void wait_ready(stIT8951_Handler_t *hdlr) {
    assert(hdlr && hdlr->get_hrdy);
    while(hdlr->get_hrdy() == 0);
}

static bool send_with_preamble(stIT8951_Handler_t *hdlr, const eIT8951_SpiPreamble_t preamble, const u16 *const data, const u32 count) {
    assert(hdlr);
    assert(IsEnum_IT8951_SpiPreamble(preamble));

    bool status = true;

    if(count == 0) {
        return true;
    }

    wait_ready(hdlr);
    hdlr->set_ncs(0);
    // Loop from -1 to send the preamble, without requiring new arr allocation
    for(i32 i=-1; i<count && status; i++) {
        const u16 txdata = (i >= 0) ? data[i] : preamble;
        status = hdlr->spi_transcieve((u16[]){__builtin_bswap16(txdata)}, NULL, sizeof(u16));
        wait_ready(hdlr);
    }
    // Ensure that the nCS is set back to the inactive state, whatever happens
    hdlr->set_ncs(1);
    return status;
}

/// @brief Sends a command to the IT8951
/// @param hdlr Pointer to the IT8951 handler
/// @param cmd Command to execute
/// @return True if the SPI transaction succeeded, false otherwise
static inline bool send_command(stIT8951_Handler_t *hdlr, const eIT8951_Command_t cmd) {
    assert(IsEnum_eIT8951_Command(cmd));
    return send_with_preamble(hdlr, IT8951_SPI_PREAMBLE_COMMAND, (u16*)&cmd, 1);
}

// TODO: write_data/write_bytes and the packed pixel transfer could be revisited
/// @brief Writes u16 words to the IT8951.
/// @param data An of u16 elements containing the data to be written.
/// @param count Number of elements to write
/// @return True if the SPI transaction succeeded, false otherwise
static inline bool write_data(stIT8951_Handler_t *hdlr, const u16 *const data, const u32 count) {
    return send_with_preamble(hdlr, IT8951_SPI_PREAMBLE_WRITE_DATA, data, count);
}

/// @brief This method should be used for non-register data transfers, where the 
/// monitoring of the HRDY pin is irrelevant. Used to transfer images
/// @param hdlr Pointer to the IT8951 handler
/// @param data Pre-formatted data bytes. Endianness depends on ImageInfo
/// @param count Number of bytes to write
/// @return True if the SPI transaction succeeded, false otherwise
static bool write_bytes(stIT8951_Handler_t *hdlr, const u8 *const data, const u32 count) {
    assert(hdlr && data);

    const size_t txsize = count*sizeof(u8)+sizeof(u16); 
    u8 *txdata = malloc(txsize);
    if(!txdata) {
        return false;
    }
    const u16 preamble = __builtin_bswap16(IT8951_SPI_PREAMBLE_WRITE_DATA);
    memcpy(&txdata[0], &preamble, sizeof(u16));
    memcpy(&txdata[2], data, count*sizeof(u8));

    wait_ready(hdlr);
    hdlr->set_ncs(0);
    // Transmit the preamble, and if successful, transmit the data as bytes
    const bool status = hdlr->spi_transcieve(&txdata, NULL, txsize);
    hdlr->set_ncs(1);
    free(txdata);
    return status;
}

/// @brief Sends a command with arguments
/// @return True if the SPI transaction succeeded, false otherwise
/// @param hdlr Pointer to the IT8951 handler
/// @param cmd Command to execute
/// @param data Data to send 
/// @param count Number of words to write
/// @return True if the SPI transaction succeeded, false otherwise
static inline bool send_command_args(stIT8951_Handler_t *hdlr, const eIT8951_Command_t cmd, const u16 *const data, const u32 count) {
    return send_command(hdlr, cmd) && write_data(hdlr, data, count);
}

/// @brief Reads the specified number of 16bit words from the IT8951
/// @param hdlr Pointer to the IT8951 handler
/// @param data Output buffer to store the data in. Must have enough space to 
/// store count*2byte of data.
/// @param count Number of 16bit words to read
/// @return True if the SPI transaction succeeded, false otherwise
static bool read_data(stIT8951_Handler_t *hdlr, u16 *const data, const u32 count) {
    assert(hdlr && data);
    
    bool status = true;

    // If nothing to read, we're done
    if(count == 0) {
        return true;
    }

    wait_ready(hdlr);
    hdlr->set_ncs(0);
    for(i32 i=-2; i<count && status; i++) {
        const u16 txdata = (i == -2) ? 0 : __builtin_bswap16(IT8951_SPI_PREAMBLE_READ_DATA);
        u16 rxdata;
        status = hdlr->spi_transcieve(&txdata, &rxdata, sizeof(u16));
        // The first 2xu16 words are discarded (preamble reply and dummy word)
        if(i >= 0) {
            data[i] = __builtin_bswap16(rxdata);
        }
        wait_ready(hdlr);
    }
    hdlr->set_ncs(1);
    return status;
}

/// @brief Writes a word to a register
/// @param hdlr Pointer to the IT8951 handler
/// @param reg Register to write to
/// @param val Value to write
/// @return True if the SPI transaction succeeded, false otherwise
static inline bool write_reg(stIT8951_Handler_t *hdlr, const eIT8951_Register_t reg, const u16 val) {
    assert(IsEnum_IT8951_Register(reg));
    return send_command_args(hdlr, IT8951_COMMAND_REG_WR, (u16[]){reg, val}, 2);
}

/// @brief Reads a word from a register
/// @param hdlr Pointer to the IT8951 handler
/// @param reg Register to read from
/// @param val Buffer to store the register's value in
/// @return True if the SPI transaction succeeded, false otherwise
static inline bool read_reg(stIT8951_Handler_t *hdlr, const eIT8951_Register_t reg, u16 *const val) {
    assert(IsEnum_IT8951_Register(reg));
    return send_command_args(hdlr, IT8951_COMMAND_REG_RD, (u16[]){reg}, 1) &&
           read_data(hdlr, val, 1);
}

/// @brief Waits for the LuT engine to finish
/// @param hdlr Pointer to the IT8951 handler
/// @return True if the SPI transaction succeeded, false otherwise
static bool wait_for_display_ready(stIT8951_Handler_t *hdlr) {
    u16 status = USHRT_MAX;
    bool ret = false;

    do {
        ret = read_reg(hdlr, IT8951_REGISTER_LUTAFSR, &status);
    } while(ret && (status != 0));

    return ret;
}

static bool load_img_area_start(stIT8951_Handler_t *hdlr, const stIT8951_ImageInfo_t *const img_info, const stRectangle *const rect) {
    assert(img_info && rect);
    if(!rectangle_is_contained_within(rect, &hdlr->panel_area)) {
        printf("Rectangle \n%s\n is not within the panel area \n%s", 
               rectangle_to_string(rect, (char[53]){0}), 
               rectangle_to_string(&hdlr->panel_area, (char[53]){0}));
        return false;
    }
    const u16 args[] = {*(u16*)img_info, rect->x, rect->y, rect->width, rect->height};
    return send_command_args(hdlr, IT8951_COMMAND_LD_IMG_AREA, args, ARRAY_LENGTH(args));
}

static inline bool load_img_end(stIT8951_Handler_t *hdlr) {
    return send_command(hdlr, IT8951_COMMAND_LD_IMG_END);
}

bool it8951_set_i80_packed_mode(stIT8951_Handler_t *hdlr, const bool enable) {
    return write_reg(hdlr, IT8951_REGISTER_I80CPCR, (const u16)enable);
} 

bool it8951_sleep(stIT8951_Handler_t *hdlr) {
    return send_command(hdlr, IT8951_COMMAND_SLEEP);
}

bool it8951_standby(stIT8951_Handler_t *hdlr) {
    return send_command(hdlr, IT8951_COMMAND_STANDBY);
}

bool it8951_system_run(stIT8951_Handler_t *hdlr) {
    return send_command(hdlr, IT8951_COMMAND_SYS_RUN);
}

/// @brief Reads the VCOM voltage from the IT8951 in mV. Note that this should
/// always be a negative value.
/// @param hdlr Pointer to the IT8951 handler
/// @param vcom_mv Buffer to store the VCOM voltage in
/// @return True if the SPI transaction succeeded, false otherwise
bool it8951_get_vcom(stIT8951_Handler_t *hdlr, i32 *vcom_mv) {
    u16 vcom = USHRT_MAX;
    const bool ret = send_command_args(hdlr, IT8951_COMMAND_CMD_VCOM, (u16[]){0}, 1) &&
                     read_data(hdlr, &vcom, 1);
    *vcom_mv = -vcom;
    return ret;
}

/// @brief Sets the VCOM voltage on the IT8951
/// @param hdlr Pointer to the IT8951 handler
/// @param vcom_mv VCOM voltage in mV. Must be negative!
/// @param store_to_flash True stores the vcom_mv to flash, false only to RAM
/// @return True if the SPI transaction succeeded, false otherwise
bool it8951_set_vcom(stIT8951_Handler_t *hdlr, const i32 vcom_mv, bool store_to_flash) {
    assert(vcom_mv < 0);
    const u16 arg = (u16)store_to_flash+1;
    // VCOM must be written as -1.50 -> 1500 = 0x62C ->[0x06, 0x2C]
    return send_command_args(hdlr, IT8951_COMMAND_CMD_VCOM, (u16[]){arg, (u16)(-vcom_mv)}, 2);
}

bool it8951_set_power(stIT8951_Handler_t *hdlr, const bool on) {
    return send_command_args(hdlr, IT8951_COMMAND_POWER_SEQUENCE, (u16[]){on}, 1);
}

bool it8951_get_device_info(stIT8951_Handler_t *hdlr, stIT8951_DeviceInfo_t *dev_info) {
    return send_command(hdlr, IT8951_COMMAND_GET_DEV_INFO) &&
           read_data(hdlr, (u16*)dev_info, sizeof(stIT8951_DeviceInfo_t)/2);
}

bool it8951_display_area(stIT8951_Handler_t *hdlr, const stRectangle *const rect, eIT8951_DisplayMode_t display_mode){
    const u16 args[] = {rect->x, rect->y, rect->width, rect->height, display_mode};
    return wait_for_display_ready(hdlr) && 
           send_command_args(hdlr, IT8951_COMMAND_DPY_AREA, args, ARRAY_LENGTH(args));
}

/// @brief Fills the specified rectangle with a uniform color. 
/// @param hdlr Pointer to the IT8951 handler
/// @param rect Boundary rectangle to fill. It must be within the display's area
/// @param mode Display mode to use for the update
/// @param colour Colour to fill the rectangle with. Must be <= 8bpp
/// @return True if the SPI transaction succeeded, false otherwise
bool it8951_fill_rect(stIT8951_Handler_t *hdlr, const stRectangle *const rect, eIT8951_DisplayMode_t mode, u8 colour){
    assert(rect);
    assert(IsEnum_IT8951_DisplayMode(mode));

    if(!rectangle_is_contained_within(rect, &hdlr->panel_area)){
        printf("Rectangle \n%s\n is not within the panel area \n%s", 
               rectangle_to_string(rect, (char[53]){0}), 
               rectangle_to_string(&hdlr->panel_area, (char[53]){0}));
        return false;
    }
    // Refresh EPD and change image buffer content with the assigned colour
    const u16 arg4 = 0x1100 | mode;
    const u16 args[] = {rect->x, rect->y, rect->width, rect->height, arg4, colour};
    return send_command_args(hdlr, IT8951_COMMAND_DPY_AREA, args, ARRAY_LENGTH(args));
}

/// @brief Fixes the IT8951's temperature sensor reading to the specified value
/// @param hdlr Pointer to the IT8951 handler
/// @param temp Temperature in C
/// @return True if the SPI transaction succeeded, false otherwise
bool it8951_force_set_temperature(stIT8951_Handler_t *hdlr, const u16 temp) {
    return send_command_args(hdlr, IT8951_COMMAND_CMD_TEMPERATURE, (u16[]){1, temp}, 2);
}

/// @brief Gets the real and forced temperature readings from the IT8951.
/// @param hdlr 
/// @param temp Array to store 2 temperature readings in [C]. The first element
/// is the real temperature, and the second is the forced one. If the temperature
/// value is forced, the 2nd temp element is meaningless
/// @return True if the SPI transaction succeeded, false otherwise
bool it8951_get_temperature(stIT8951_Handler_t *hdlr, u16 *temp) {
    return send_command_args(hdlr, IT8951_COMMAND_CMD_TEMPERATURE, (u16[]){0}, 1) &&
           // TODO: The datasheet is ambiguous about the order of the real and 
           // forced temperatures
           read_data(hdlr, temp, 2);
}

/// @brief After a forced (fixed) temperature settings this command ensures that
/// the IT8951 continues to read the real temperature sensor.
bool it8951_cancel_force_temperature(stIT8951_Handler_t *hdlr) {
    return send_command_args(hdlr, IT8951_COMMAND_CMD_TEMPERATURE, (u16[]){2}, 1);
}

/// @brief This command was designed for 2 bpp image display. Without this 
/// command, 2bpp cannot show correct white pixel. Host should call this command
/// before display 2bpp image
/// @param hdlr Pointer to the IT8951 handler
/// @param is_2bpp True for 2bpp, false for all other
/// @return True if the SPI transaction succeeded, false otherwise
bool it8951_set_bpp_mode(stIT8951_Handler_t *hdlr, bool is_2bpp) {
    return send_command_args(hdlr, IT8951_COMMAND_BPP_SETTINGS, (u16[]){is_2bpp}, 1);
}

bool it8951_set_img_buff_base_address(stIT8951_Handler_t *hdlr, const u32 addr) {
    // The address must be <26bits
    assert(addr < (1UL << 26));
    const u16 addr_h = (u16)(addr >> 16);
    const u16 addr_l = (u16)(addr);
    return write_reg(hdlr, IT8951_REGISTER_LISAR,   addr_l) &&
           write_reg(hdlr, IT8951_REGISTER_LISAR+2, addr_h);
}

/// @brief Writes the specified pixels to the IT8951's internal frame buffer but
/// does not render the image on the screen. Call @ref it8951_display_area after
/// writing the pixels to display on the EPD
/// @param hdlr Pointer to the IT8951 handler 
/// @param img_info Pointer to the Image Info struct
/// @param rect Pointer to the rectangle on the screen to write the pixels to
/// @param ppixels Pointer to the packed pixels to write
/// @return True if the SPI transaction succeeded, false otherwise
bool it8951_write_packed_pixels(stIT8951_Handler_t *hdlr, const stIT8951_ImageInfo_t *const img_info, const stRectangle *const rect, const u16 *const ppixels, const u32 count) {
    assert(hdlr && img_info && rect && ppixels);
    // TODO: Other bpps are not yet supported
    assert(img_info->bpp == IT8951_COLOR_DEPTH_BPP_4BIT);

    if(!rectangle_is_contained_within(rect, &hdlr->panel_area)){
        printf("Rectangle \n%s\n is not within the panel area \n%s", 
               rectangle_to_string(rect, (char[53]){0}), 
               rectangle_to_string(&hdlr->panel_area, (char[53]){0}));
        return false;
    }

    return load_img_area_start(hdlr, img_info, rect) && 
           write_bytes(hdlr, (u8*)ppixels, count) && 
           load_img_end(hdlr);
}

// TODO: Although BMP files naturally have padding and are already packed, these
// images could be uploaded to an offset x, y coordinate on the IT8951's screen, 
// which would require different padding.
/// @brief Loads a 4bpp BMP file onto the IT8951's screen at the specified location
/// @param hdlr Pointer to the IT8951 handler
/// @param bmp Path to the BMP file
/// @param x X coordinate to load the BMP at
/// @param y Y coordinate to load the BMP at
/// @return 
bool it8951_load_bmp(stIT8951_Handler_t *hdlr, const char *const bmp, const u16 x, const u16 y){
    FILE *img = fopen(bmp, "rb");
    if(img == NULL){
        printf("Failed to open the BMP file");
        return false;
    } else {
        // Read the header. See https://en.wikipedia.org/wiki/BMP_file_format)
        char header[2];
        fread(header, sizeof(char), 2, img);
        if(header[0] != 'B' || header[1] != 'M'){
            printf("Invalid BMP file");
            fclose(img);
            return false;
        }

        u32 pix_arr_offset, width, height;
        u16 bpp;
        // byte [10:13] encodes the bitmap byte array offset in the image
        // byte [10:13] encodes the bitmap width in pixels
        // byte [22:25] encodes the bitmap height in pixels
        // byte [28:29] encodes the bitmap bpp
        fseek(img, 10, SEEK_SET);
        fread(&pix_arr_offset, sizeof(pix_arr_offset), 1, img);

        fseek(img, 18, SEEK_SET);
        fread(&width, sizeof(width), 1, img);
        fread(&height, sizeof(height), 1, img);
        
        fseek(img, 28, SEEK_SET);
        fread(&bpp, sizeof(bpp), 1, img);
        // TODO: Other bpps are not yet supported
        assert(bpp == 4);

        const size_t img_size = (width*height*bpp)/8;
        u8 *buff = malloc(img_size);
        fseek(img, pix_arr_offset, SEEK_SET);
        fread(buff, sizeof(*buff), img_size, img);
        fclose(img);

        stRectangle rect = {x, y, width, height};
        stIT8951_ImageInfo_t img_info = {
            .rotation   = IT8951_ROTATION_MODE_0,
            .bpp        = IT8951_COLOR_DEPTH_BPP_4BIT,
            .endianness = IT8951_ENDIANNESS_LITTLE,
        };
        // If the BMP's width/height cannot fit on the screen, this will fail.
        return it8951_write_packed_pixels(hdlr, &img_info, &rect, (u16*)buff, img_size/2);
    }
}

/// @brief Packs the pixels contained in a u8 array into 16bit words, that can
/// directly be transferred to the IT8951
/// @param img_info Pointer to the Image Info struct
/// @param rect Rectangle to pack the pixels into.
/// @param in_pix Must be at least rect->width*rect->height long. Pixels outside
/// the rectangle are ignored
/// @param out_words [out] Must be at least (x+start_pad+end_pad)/4*y long and
/// can be left uninitialized by the caller
void it8951_pack_pixels(stIT8951_ImageInfo_t *img_info, const stRectangle *const rect, const u8 *const in_pix, u16 *const out_words) {
    // TODO: Generalise it to other bpps
    // The following alignment rules must be met:
    // 2bpp -> start_x % 8 = 0, end_x % 8 = 0
    // 4bpp -> start_x % 4 = 0, end_x % 4 = 0
    // 8bpp -> start_x % 2 = 0, end_x % 2 = 0
    assert(img_info && rect && in_pix && out_words);
    // TODO: Other bpps are not yet supported
    assert(img_info->bpp == IT8951_COLOR_DEPTH_BPP_4BIT);

    const u32 pix_per_byte  = it8951_get_pixel_per_byte(img_info->bpp);

    if(img_info->bpp == IT8951_COLOR_DEPTH_BPP_4BIT && img_info->endianness == IT8951_ENDIANNESS_LITTLE) {
        const u32 end_mod   = (rect->x + rect->width) % pix_per_byte;
        const u32 start_pad = rect->x % pix_per_byte;
        const u32 end_pad   = (end_mod == 0) ? 0 : (pix_per_byte - end_mod);
        // (x+start_pad+end_pad)/4*y is the number of u16 words the rectangle is 
        // encoded on, taking the alignment rules into account
        const u32 word_cnt = ((rect->width+start_pad+end_pad)*rect->height)/pix_per_byte;
        const u32 word_per_row = word_cnt/rect->height;

        // Ensure that the padding is zeroed out
        memset(out_words, 0, word_cnt*sizeof(*out_words));

        // Note: in_pix will naturally be only accessed in the valid range due 
        // to the padding
        u32 idx = 0;
        for(u32 w=0; w<word_cnt; w++) {
            // If this is the first word in the row, pad the beginning
            const u32 mod = w % word_per_row;
            u32 low_bound, high_bound;
            if(mod == 0){
                low_bound  = start_pad;
                high_bound = pix_per_byte;
            // If this is the last word in the row, pad the end
            } else if(mod == word_per_row-1) {
                low_bound  = 0;
                high_bound = end_pad;
            // Else just copy as usual
            } else {
                low_bound  = 0;
                high_bound = pix_per_byte;
            }
            for(u32 p=low_bound; p<high_bound; p++) {
                out_words[w] |= ((u16)in_pix[idx++] << (4*p));
            }
        }
    }
}

// For the SPI protocol description, refer to 
// https://www.waveshare.net/w/upload/1/18/IT8951_D_V0.2.4.3_20170728.pdf and
// https://v4.cecdn.yun300.cn/100001_1909185148/IT8951_I80+ProgrammingGuide_16bits_20170904_v2.7_common_CXDX.pdf
/// @brief Initialises the IT8951 and optionally modifies the vcom voltage
/// @param hdlr Pointer to the handler struct
/// @return True if the SPI transaction succeeded, false otherwise
bool it8951_init(stIT8951_Handler_t *hdlr) {
    printf("Initialising IT8951...");

    if(!it8951_get_device_info(hdlr, &hdlr->device_info) || 
       hdlr->device_info.panel_height == 0 || hdlr->device_info.panel_width)
    {
        goto Terminate;
    }

    char dev_info_str[256];
    printf("%s", it8951_device_info_to_string(&hdlr->device_info, dev_info_str));
    
    if(!it8951_set_img_buff_base_address(hdlr, hdlr->device_info.img_buff_addr))
        goto Terminate;
    if(!it8951_set_i80_packed_mode(hdlr, true))
        goto Terminate;
    hdlr->panel_area = (stRectangle){
        .x      = 0,
        .y      = 0,
        .width  = hdlr->device_info.panel_width,
        .height = hdlr->device_info.panel_height
    };

    i32 rxvcom_mv = 0; 
    if(!it8951_get_vcom(hdlr, &rxvcom_mv))
        goto Terminate;

    printf("Current VCOM = %f", (rxvcom_mv/1000.0f));

    if(hdlr->vcom_mv != INT_MAX && rxvcom_mv != hdlr->vcom_mv){
        printf("Setting VCOM to user-specified %f", (hdlr->vcom_mv/1000.0f));
        if(!it8951_set_vcom(hdlr, hdlr->vcom_mv, false))
            goto Terminate;
    }
    
    return true;
Terminate:
    printf("Communication with the IT8951 over SPI failed");
    return false;
}
