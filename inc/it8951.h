#ifndef __IT8951_H__
#define __IT8951_H__

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

// TODO: Put to utilites.h
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int32_t  i32;
typedef int16_t  i16;

#define IS_WITHIN_RANGE(x, low, high) ((x) >= (low) && (x) <= (high))
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))
#define CEIL_DIV2(x) (((x) + 1) >> 2)

// TODO: Consider storing all the enums as a byte swapped version to save some 
// clock cycles at transmission 

enum eIT8951_RegisterBase {
    IT8951_REGISTER_BASE_USB         = 0x4E00,
    IT8951_REGISTER_BASE_I2C         = 0x4C00,
    IT8951_REGISTER_BASE_IMAGE_PROC  = 0x4600,
    IT8951_REGISTER_BASE_JPG         = 0x4000,
    IT8951_REGISTER_BASE_GPIO        = 0x1E00,
    IT8951_REGISTER_BASE_HS_UART     = 0x1C00,
    IT8951_REGISTER_BASE_INTC        = 0x1400,
    IT8951_REGISTER_BASE_DISP_CTRL   = 0x1000,
    IT8951_REGISTER_BASE_SPI         = 0x0E00,
    IT8951_REGISTER_BASE_THERM       = 0x0800,
    IT8951_REGISTER_BASE_SD_CARD     = 0x0600,
    IT8951_REGISTER_BASE_MEMORY_CONV = 0x0200,
    IT8951_REGISTER_BASE_SYSTEM      = 0x0000,
};

/// @brief IT8952 register memory map
typedef enum eIT8951_Register {
    /// @brief Engine width/height register
    IT8951_REGISTER_LUT0EWHR  = IT8951_REGISTER_BASE_DISP_CTRL + 0x000,
    /// @brief LUT0 XY register
    IT8951_REGISTER_LUT0XYR   = IT8951_REGISTER_BASE_DISP_CTRL + 0x040,
    /// @brief LUT0 Base Address Reg
    IT8951_REGISTER_LUT0BADDR = IT8951_REGISTER_BASE_DISP_CTRL + 0x080,
    /// @brief LUT0 Mode and Frame number Reg
    IT8951_REGISTER_LUT0MFN   = IT8951_REGISTER_BASE_DISP_CTRL + 0x0C0,
    /// @brief LUT0 and LUT1 Active Flag Reg
    IT8951_REGISTER_LUT01AF   = IT8951_REGISTER_BASE_DISP_CTRL + 0x114,
    /// @brief Update parameter0 setting reg
    IT8951_REGISTER_UP0SR     = IT8951_REGISTER_BASE_DISP_CTRL + 0x134,
    /// @brief Update parameter1 setting reg
    IT8951_REGISTER_UP1SR     = IT8951_REGISTER_BASE_DISP_CTRL + 0x138,
    /// @brief LUT0 Alpha blend and fill rectangle value
    IT8951_REGISTER_LUT0ABFRV = IT8951_REGISTER_BASE_DISP_CTRL + 0x13C,
    /// @brief Update buffer base address
    IT8951_REGISTER_UPBBADDR  = IT8951_REGISTER_BASE_DISP_CTRL + 0x17C,
    /// @brief LUT0 Image buffer X/Y offset register
    IT8951_REGISTER_LUT0IMXY  = IT8951_REGISTER_BASE_DISP_CTRL + 0x180,
    /// @brief LUT Status Reg (status of All LUT Engines)
    IT8951_REGISTER_LUTAFSR   = IT8951_REGISTER_BASE_DISP_CTRL + 0x224,
    /// @brief Set BG and FG Color if Bitmap mode enable only (1bpp)
    IT8951_REGISTER_BGVR      = IT8951_REGISTER_BASE_DISP_CTRL + 0x250,

    IT8951_REGISTER_I80CPCR   = IT8951_REGISTER_BASE_SYSTEM    + 0x004,

    IT8951_REGISTER_MCSR      = IT8951_REGISTER_BASE_MEMORY_CONV + 0x00,
    IT8951_REGISTER_LISAR     = IT8951_REGISTER_BASE_MEMORY_CONV + 0x08,
} eIT8951_Register_t;
#define IsEnum_IT8951_Register(e) ((e) == IT8951_REGISTER_LUT0EWHR  || \
                                   (e) == IT8951_REGISTER_LUT0XYR   || \
                                   (e) == IT8951_REGISTER_LUT0BADDR || \
                                   (e) == IT8951_REGISTER_LUT0MFN   || \
                                   (e) == IT8951_REGISTER_LUT01AF   || \
                                   (e) == IT8951_REGISTER_UP0SR     || \
                                   (e) == IT8951_REGISTER_UP1SR     || \
                                   (e) == IT8951_REGISTER_LUT0ABFRV || \
                                   (e) == IT8951_REGISTER_UPBBADDR  || \
                                   (e) == IT8951_REGISTER_LUT0IMXY  || \
                                   (e) == IT8951_REGISTER_LUTAFSR   || \
                                   (e) == IT8951_REGISTER_BGVR      || \
                                   (e) == IT8951_REGISTER_I80CPCR   || \
                                   (e) == IT8951_REGISTER_MCSR      || \
                                   (e) == IT8951_REGISTER_LISAR)

typedef enum eIT8951_CommsMode {
    /// @brief TEST_CFG[2:0] = 0b000, I80CPCR = 0 (not supported)
    IT8951_COMMS_MODE_I80      = 0,
    /// @brief TEST_CFG[2:0] = 0b000, I80CPCR = 1 (not supported)
    IT8951_COMMS_MODE_M68      = 1,
    /// @brief TEST_CFG[2:0] = 0b001, I80CPCR = X
    IT8951_COMMS_MODE_SPI      = 2,
    /// @brief TEST_CFG[2:0] = 0b110, I80CPCR = X (not supported)
    IT8951_COMMS_MODE_I2C_0x46 = 3,
    /// @brief TEST_CFG[2:0] = 0b111, I80CPCR = X (not supported)
    IT8951_COMMS_MODE_I2C_0x35 = 4,
} eIT8951_CommsMode_t;

typedef enum eIT8951_SpiPreamble {
    IT8951_SPI_PREAMBLE_COMMAND    = 0x6000,
    IT8951_SPI_PREAMBLE_WRITE_DATA = 0x0000,
    IT8951_SPI_PREAMBLE_READ_DATA  = 0x1000
} eIT8951_SpiPreamble_t;
#define IsEnum_IT8951_SpiPreamble(e) ((e) == IT8951_SPI_PREAMBLE_COMMAND    || \
                                      (e) == IT8951_SPI_PREAMBLE_WRITE_DATA || \
                                      (e) == IT8951_SPI_PREAMBLE_READ_DATA)

typedef enum eIT8951_Command {
    /// @brief System running command: enable all clocks, and go to active state
    IT8951_COMMAND_SYS_RUN         = 0x0001,
    /// @brief Standby command: gate off clocks, and go to standby state
    IT8951_COMMAND_STANDBY         = 0x0002,
    /// @brief Sleep command: disable all clocks, and go to sleep state
    IT8951_COMMAND_SLEEP           = 0x0003,
    /// @brief Read register command
    IT8951_COMMAND_REG_RD          = 0x0010,
    /// @brief Write register command
    IT8951_COMMAND_REG_WR          = 0x0011,
    IT8951_COMMAND_MEM_BST_RD_T    = 0x0012,
    IT8951_COMMAND_MEM_BST_RD_S    = 0x0013,
    IT8951_COMMAND_MEM_BST_WR      = 0x0014,
    IT8951_COMMAND_MEM_BST_END     = 0x0015,
    IT8951_COMMAND_LD_IMG          = 0x0020,
    IT8951_COMMAND_LD_IMG_AREA     = 0x0021,
    IT8951_COMMAND_LD_IMG_END      = 0x0022,
    IT8951_COMMAND_LD_IMG_1BPP     = 0x0095,
    IT8951_COMMAND_DPY_AREA        = 0x0034,
    IT8951_COMMAND_DPY_BUF_AREA    = 0x0037,
    IT8951_COMMAND_POWER_SEQUENCE  = 0x0038,
    IT8951_COMMAND_CMD_VCOM        = 0x0039,
    IT8951_COMMAND_FILL_RECT       = 0x003A,
    IT8951_COMMAND_CMD_TEMPERATURE = 0x0040,
    IT8951_COMMAND_BPP_SETTINGS    = 0x0080,
    IT8951_COMMAND_GET_DEV_INFO    = 0x0302
} eIT8951_Command_t;
#define IsEnum_eIT8951_Command(e) ((e) == IT8951_COMMAND_SYS_RUN         || \
                                   (e) == IT8951_COMMAND_STANDBY         || \
                                   (e) == IT8951_COMMAND_SLEEP           || \
                                   (e) == IT8951_COMMAND_REG_RD          || \
                                   (e) == IT8951_COMMAND_REG_WR          || \
                                   (e) == IT8951_COMMAND_MEM_BST_RD_T    || \
                                   (e) == IT8951_COMMAND_MEM_BST_RD_S    || \
                                   (e) == IT8951_COMMAND_MEM_BST_WR      || \
                                   (e) == IT8951_COMMAND_MEM_BST_END     || \
                                   (e) == IT8951_COMMAND_LD_IMG          || \
                                   (e) == IT8951_COMMAND_LD_IMG_AREA     || \
                                   (e) == IT8951_COMMAND_LD_IMG_END      || \
                                   (e) == IT8951_COMMAND_LD_IMG_1BPP     || \
                                   (e) == IT8951_COMMAND_DPY_AREA        || \
                                   (e) == IT8951_COMMAND_DPY_BUF_AREA    || \
                                   (e) == IT8951_COMMAND_POWER_SEQUENCE  || \
                                   (e) == IT8951_COMMAND_CMD_VCOM        || \
                                   (e) == IT8951_COMMAND_FILL_RECT       || \
                                   (e) == IT8951_COMMAND_CMD_TEMPERATURE || \
                                   (e) == IT8951_COMMAND_BPP_SETTINGS    || \
                                   (e) == IT8951_COMMAND_GET_DEV_INFO)

/// @brief Color-depth of the display to drive
typedef enum eIT8951_ColorDepth {
    /// @brief |P[n+7]|P[n+6]|P[n+5]|P[n+4]|P[n+3]|P[n+2]|P[n+1]|P[n+0]|
    IT8951_COLOR_DEPTH_BPP_2BIT = 0,
    /// @brief |P[n+3] 0|P[n+2] 0|P[n+1] 0|P[n+0] 0|
    IT8951_COLOR_DEPTH_BPP_3BIT = 1,
    /// @brief |P[n+3]|P[n+2]|P[n+1]|P[n+0]|
    IT8951_COLOR_DEPTH_BPP_4BIT = 2,
    /// @brief |P[n+1]|P[n+0]|
    IT8951_COLOR_DEPTH_BPP_8BIT = 3,
    IT8951_COLOR_DEPTH_BPP_1BIT = 4
} eIT8951_ColorDepth_t;
#define IsEnum_IT8951_ColorDepth(e) ((e) <= IT8951_COLOR_DEPTH_BPP_1BIT)
#define IT8951_BPP_PER_BYTE_MAP ((u32[]){4, 2, 2, 1, 8})
#define IT8951_BPP_CODE_MAP ((i32[]){-1,                          \
                                     IT8951_COLOR_DEPTH_BPP_1BIT, \
                                     IT8951_COLOR_DEPTH_BPP_2BIT, \
                                     IT8951_COLOR_DEPTH_BPP_3BIT, \
                                     IT8951_COLOR_DEPTH_BPP_4BIT, \
                                     -1, -1, -1,                  \
                                     IT8951_COLOR_DEPTH_BPP_8BIT})

/// @brief Rotational angle of the displayed image
typedef enum eIT8951_RotationMode {
    IT8951_ROTATION_MODE_0   = 0,
    IT8951_ROTATION_MODE_90  = 1,
    IT8951_ROTATION_MODE_180 = 2,
    IT8951_ROTATION_MODE_270 = 3
} eIT8951_RotationMode_t;

typedef enum eIT8951_Endianness {
    IT8951_ENDIANNESS_LITTLE = 0,
    IT8951_ENDIANNESS_BIG    = 1
} eIT8951_Endianness_t;

/// @brief Waveform display modes are described here:
/// http://www.waveshare.net/w/upload/c/c4/E-paper-mode-declaration.pdf
typedef enum eIT8951_DisplayMode {
    IT8951_DISPLAY_MODE_INIT  = 0,
    IT8951_DISPLAY_MODE_DU    = 1,
    IT8951_DISPLAY_MODE_GC16  = 2,
    IT8951_DISPLAY_MODE_GL16  = 3,
    IT8951_DISPLAY_MODE_GLR16 = 4,
    IT8951_DISPLAY_MODE_GLD16 = 5,
    IT8951_DISPLAY_MODE_A2    = 6,
    IT8951_DISPLAY_MODE_DU4   = 7
} eIT8951_DisplayMode_t;
#define IsEnum_IT8951_DisplayMode(e) ((e) <= IT8951_DISPLAY_MODE_DU4)

typedef struct __attribute__((packed, aligned(2))) stIT8951_DeviceInfo {
    u16 panel_width;
    u16 panel_height;
    u32 img_buff_addr;
    char firmware_version[16];
    char lut_version[16];
} stIT8951_DeviceInfo_t;
static_assert(sizeof(stIT8951_DeviceInfo_t) == 40);

// TODO: Check if this is u16
typedef struct __attribute__((packed)) stRectangle {
    u16 x, y, width, height;
} stRectangle;
static_assert(sizeof(stRectangle) == 8);

// TODO: Ensure that the bit order is correct. This is undefined in C!
typedef struct __attribute__((packed)) stIT8951_ImageInfo {
    eIT8951_RotationMode_t rotation   : 4;
    eIT8951_ColorDepth_t   bpp        : 4;
    eIT8951_Endianness_t   endianness : 8;
} stIT8951_ImageInfo_t;
static_assert(sizeof(stIT8951_ImageInfo_t) == 2);

typedef struct stIT8951_Handler {
    /// @brief Pointer to the SPI transcieve function. The SPI peripheral must
    /// be initialised to <24MHz clock before initing the IT8951. TODO: Get the
    /// SPI mode here. Note: if either of the txdata, rxdata pointers are null,
    /// the transcieve function must discard the corresponding data
    bool (*spi_transcieve)(const void *txdata, void *rxdata, size_t len);
    /// @brief Pointer to the function that sets the nCS GPIO to the required
    /// state.
    void (*set_ncs)(bool state);
    /// @brief Pointer to the function that reads the HRDY GPIO's state
    bool (*get_hrdy)(void);
    /// @brief VCOM voltage level in mV. Usually its a negative value. Set to 
    /// INT_MAX if the default VCOM voltage is to be kept. Note that the IT8951
    /// development boards ship with waveforms that are tuned to a specific vcom
    /// voltage. Therefore setting this may mess up the drawn pixels.
    i32 vcom_mv;
    stIT8951_DeviceInfo_t device_info;
    stRectangle panel_area;
} stIT8951_Handler_t;

bool it8951_init(stIT8951_Handler_t *hdlr);

#endif
