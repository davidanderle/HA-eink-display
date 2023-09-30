import sys
# TODO: Remove as not compatible with uP
from array import array
if sys.platform == "esp32":
    import pros3
    from machine import Pin, SPI
if sys.platform == "win32":
    from unittest.mock import Mock
    class SPI:
        pass
    class Pin:
        pass

# IT8951 memory map
class RegisterBase:
    USB         = 0x4E00
    I2C         = 0x4C00
    IMAGE_PROC  = 0x4600
    JPG         = 0x4000
    GPIO        = 0x1E00
    HS_UART     = 0x1C00
    INTC        = 0x1400
    DISP_CTRL   = 0x1000
    SPI         = 0x0E00
    THERM       = 0x0800
    SD_CARD     = 0x0600
    MEMORY_CONV = 0x0200
    SYSTEM      = 0x0000
    
# IT8951 register memory map (incomplete)
class Register:
    # Engine width/height register
    LUT0EWHR  = RegisterBase.DISP_CTRL + 0x000
    # LUT0 XY register
    LUT0XYR   = RegisterBase.DISP_CTRL + 0x040
    # LUT0 Base Address Reg
    LUT0BADDR = RegisterBase.DISP_CTRL + 0x080
    # LUT0 Mode and Frame number Reg
    LUT0MFN   = RegisterBase.DISP_CTRL + 0x0C0 
    # LUT0 and LUT1 Active Flag Reg
    LUT01AF   = RegisterBase.DISP_CTRL + 0x114 
    # Update parameter0 setting reg
    UP0SR     = RegisterBase.DISP_CTRL + 0x134
    # Update parameter1 setting reg
    UP1SR     = RegisterBase.DISP_CTRL + 0x138
    # LUT0 Alpha blend and fill rectangle value
    LUT0ABFRV = RegisterBase.DISP_CTRL + 0x13C
    # Update buffer base address
    UPBBADDR  = RegisterBase.DISP_CTRL + 0x17C
    # LUT0 Image buffer X/Y offset register
    LUT0IMXY  = RegisterBase.DISP_CTRL + 0x180
    # LUT Status Reg (status of All LUT Engines)
    LUTAFSR   = RegisterBase.DISP_CTRL + 0x224
    # Set BG and FG Color if Bitmap mode enable only (1bpp)
    BGVR      = RegisterBase.DISP_CTRL + 0x250

    I80CPR    = RegisterBase.SYSTEM    + 0x004

    MCSR      = RegisterBase.MEMORY_CONV + 0x00
    LISAR     = RegisterBase.MEMORY_CONV + 0x08

class CommsMode:
    # TEST_CFG[2:0] = 0b000, I80CPCR = 0 (not supported)
    I80      = 0
    # TEST_CFG[2:0] = 0b000, I80CPCR = 1 (not supported)
    M68      = 1
    # TEST_CFG[2:0] = 0b001, I80CPCR = X
    SPI      = 2
    # TEST_CFG[2:0] = 0b110, I80CPCR = X (not supported)
    I2C_0x46 = 3
    # TEST_CFG[2:0] = 0b111, I80CPCR = X (not supported)
    I2C_0x35 = 4

# These double-words are used in an SPI frame to set their type
class SpiPreamble:
    COMMAND    = 0x6000
    WRITE_DATA = 0x0000
    READ_DATA  = 0x1000
    
class Command:
    # System running command: enable all clocks, and go to active state
    SYS_RUN         = 0x0001
    # Standby command: gate off clocks, and go to standby state
    STANDBY         = 0x0002
    # Sleep command: disable all clocks, and go to sleep state
    SLEEP           = 0x0003
    # Read register command
    REG_RD          = 0x0010
    # Write register command
    REG_WR          = 0x0011
    MEM_BST_RD_T    = 0x0012
    MEM_BST_RD_S    = 0x0013
    MEM_BST_WR      = 0x0014
    MEM_BST_END     = 0x0015
    LD_IMG          = 0x0020
    LD_IMG_AREA     = 0x0021
    LD_IMG_END      = 0x0022
    LD_IMG_1BPP     = 0x0095
    DPY_AREA        = 0x0034
    DPY_BUF_AREA    = 0x0037
    POWER_SEQUENCE  = 0x0038
    CMD_VCOM        = 0x0039
    FILL_RECT       = 0x003A
    CMD_TEMPERATURE = 0x0040
    BPP_SETTINGS    = 0x0080
    GET_DEV_INFO    = 0x0302

# Color-depth of the display to drive
class ColorDepth:
    BPP_2BIT = 0
    BPP_3BIT = 1
    BPP_4BIT = 2
    BPP_8BIT = 3

# Rotational angle of the displayed image
class RotateMode:
    ROTATE_0   = 0
    ROTATE_90  = 1
    ROTATE_180 = 2
    ROTATE_270 = 3
    
# Waveform display modes are described here:
# http://www.waveshare.net/w/upload/c/c4/E-paper-mode-declaration.pdf
class DisplayModes:
    INIT  = 0
    DU    = 1
    GC16  = 2
    GL16  = 3
    GLR16 = 4
    GLD16 = 5
    A2    = 6
    DU4   = 7
    
class DeviceInfo:
    Size = 40
    def __init__(self, panel_width: int, panel_height: int, img_buff_addr: int,\
                 firmware_version: str, lut_version: str):
        self.panel_width      = panel_width
        self.panel_height     = panel_height
        self.img_buff_addr    = img_buff_addr
        self.firmware_version = firmware_version
        self.lut_version      = lut_version
    
    @classmethod
    def from_u16_words(cls, u16_words: list):
        if len(u16_words)*2 != DeviceInfo.Size:
            raise "Input must have 20 elements (40 bytes)"

        width            = u16_words[0]
        height           = u16_words[1]
        img_buff_addr    = (u16_words[2] << 16) | u16_words[3]
        firmware_version = ''.join(chr(word & 0xFF) + chr((word >> 8) & 0xFF)\
                                   for word in u16_words[4:11])
        lut_version      = ''.join(chr(word & 0xFF) + chr((word >> 8) & 0xFF)\
                                   for word in u16_words[12:19])

        return cls(
            width,
            height,
            img_buff_addr,
            firmware_version,
            lut_version
        )

    def __str__(self):
        return f"Panel width: {self.panel_width}\n" + \
               f"Panel height: {self.panel_height}\n" + \
               f"Image buffer address: {hex(self.img_buff_addr)}\n" + \
               f"Firmware version: {self.firmware_version}\n" + \
               f"LUT version: {self.lut_version}"

# For the SPI protocol description, refer to 
# https://www.waveshare.net/w/upload/1/18/IT8951_D_V0.2.4.3_20170728.pdf and
# https://v4.cecdn.yun300.cn/100001_1909185148/IT8951_I80+ProgrammingGuide_16bits_20170904_v2.7_common_CXDX.pdf
class it8951:
    def __init__(self, spi: SPI, ncs: Pin, hrdy: Pin):
        # There are 2 hardware SPI channels on the ESP32 and they can be mapped
        # to any pin, however, they are limited to 40MHz if not used on the 
        # default ones. The IT8951's maximum SPI speed is 24MHz either way, so 
        # we're good. The data must be clocked out MSB first at SPI Mode 0 or 3 
        # (0 used). At 24MHz a full refresh should take about <450ms
        self._spi = spi
        # Active-low chip select
        self._ncs = ncs
        # Host-ready pin (toggled by the IT8951)
        self._hrdy = hrdy
    
    def _wait_ready(self):
        """
        The host must wait for the HRDY pin to be high before Tx/Rx of the next 
        2x8 bits.
        """
        while self._hrdy.value() == 0: pass

    def _send_command(self, command: Command):
        """
        Sends a command to the IT8951.
        Args:
            command: Command to execute
        """
        try:
            txdata = [SpiPreamble.COMMAND, command]
            self._wait_ready()
            self._ncs(0)
            for i in range(len(txdata)):
                self._wait_ready()
                self._spi.write(txdata[i].to_bytes(2, 'big'))
        finally:
            self._ncs(1)
    
    def _write_data(self, data: list):
        """
        Writes u16 words to the IT8951.
        Args: 
            data: A list of u16 elements containing the data to be written
        """
        if not data: return

        try:
            txdata = [SpiPreamble.WRITE_DATA] + data
            self._wait_ready()
            self._ncs(0)
            for i in range(len(txdata)):
                self._wait_ready()
                self._spi.write(txdata[i].to_bytes(2, 'big'))
        finally:
            self._ncs(1)

    def _send_command_args(self, command: Command, args: list):
        self._send_command(command)
        self._write_data(args)

    # TODO: Fix this using the tests
    # TODO: The reading of the VCOM is not working. It also messes up subsequent
    #       reads
    def _read_data(self, length: int) -> list:
        """
        Reads the specified number of 16bit words from the IT8951.
        Args:
            length: number of u16 elements to read
        """
        if length == 0: return []
        try:
            # The first word returned from the controller is dummy:u16
            txdata = [SpiPreamble.READ_DATA] + [0]*(length+1)
            rxdata = []

            self._wait_ready()
            self._ncs(0)
            for i in range(len(txdata)):
                self._wait_ready()
                rx = self._spi.read(2, txdata[i].to_bytes(2, 'big'))
                rxdata.append((rx[0] << 8) | rx[1])

            # Take off the first 2 words (preampble and dummy words)
            return rxdata[2:]
        finally:
            self._ncs(1)
            
    def _write_reg(self, reg: Register, data: list):
        """
        Writes the specified number of words to a register
        Args:
            reg: Register to write to
            data: u16 words to write to reg
        """
        self._send_command(Command.REG_WR)
        self._write_data([reg, data])
    
    def _read_reg(self, reg: Register, length: int) -> list:
        """
        Reads the specified number of words from a register
        Args:
            reg: Register to write to
            data: u16 words to write to reg
        """
        self._send_command(Command.REG_RD)
        self._write_data(reg)
        return self._read_data(length)
    
    def sleep(self):
        self._send_command(Command.SLEEP)
        
    def standby(self):
        self._send_command(Command.STANDBY)
        
    def system_run(self):
        self._send_command(Command.SYS_RUN)
    
    def get_vcom(self) -> int:
        self._send_command_args(Command.CMD_VCOM, [0])
        rxdata = self._spi.read(2)
        return (rxdata[0] << 8) | rxdata[1]

    def set_vcom(self, vcom_mV, store_to_flash: bool = False):
        arg = 2 if store_to_flash else 1
        self._send_command_args(Command.CMD_VCOM, [arg, vcom_mV])
    
    def set_power(self, enable: bool):
        self._send_command_args(Command.POWER_SEQUENCE, [enable])
        
    def get_device_info(self) -> DeviceInfo:
        self._send_command(Command.GET_DEV_INFO)
        rxdata = self._read_data(int(DeviceInfo.Size/2))
        return DeviceInfo.from_u16_words(rxdata)
    
    def fill_rect(self, x: int, y: int, width: int, height: int, pix: int):
        # TODO: Add checks for width, height, and pix (0-255)
        self._send_command_args(Command.FILL_RECT, [x, y, width, height, pix])
        
    def force_set_temperature(self, temperature_C: int):
        """
        Fixes the IT8951's temperature sensor readings to the specified 
        temperature in C
        """
        self._send_command_args(Command.CMD_TEMPERATURE, [1, temperature_C])

    def get_temperature(self) -> (int, int):
        """
        Gets the real (0th) and forced (1st) temperature sensor reading from the
        IT8951. If the temperature value isn't fixed (forced), the 2nd touple 
        element is meaningless
        """
        self._send_command_args(Command.CMD_TEMPERATURE, [0])
        rxdata = self._spi.read(4)
        # TODO: The datasheet is ambiguous on the order of the real and forced T
        return ((rxdata[0] << 8) | rxdata[1], (rxdata[2] << 8) | rxdata[3])
    
    def cancel_force_temperature(self):
        """
        After a forced (fixed) temperature settings this command ensures that
        the IT8951 continues to read the real temperature sensor.
        """
        self._send_command_args(Command.CMD_TEMPERATURE, [2])
    
    def set_bpp_mode(self, is_2bpp: bool):
        """
        This command was designed for 2 bpp image display. Without this command,
        2bpp cannot show correct white pixel. Host should call this command
        before display 2bpp image
        """
        self._send_command_args(Command.BPP_SETTINGS, [is_2bpp])
