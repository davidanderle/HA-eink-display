import sys
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
    SYS_RUN      = 0x0001
    # Standby command: gate off clocks, and go to standby state
    STANDBY      = 0x0002
    # Sleep command: disable all clocks, and go to sleep state
    SLEEP        = 0x0003
    # Read register command
    REG_RD       = 0x0010
    # Write register command
    REG_WR       = 0x0011
    MEM_BST_RD_T = 0x0012
    MEM_BST_RD_S = 0x0013
    MEM_BST_WR   = 0x0014
    MEM_BST_END  = 0x0015
    LD_IMG       = 0x0020
    LD_IMG_AREA  = 0x0021
    LD_IMG_END   = 0x0022
    LD_IMG_1BPP  = 0x0095
    # TODO: Continue
    DPY_AREA = 0x0034
    POWER_SEQUENCE = 0x0038
    SET_VCOM = 0x0039
    GET_DEV_INFO = 0x0302
    DPY_BUF_AREA = 0x0037

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

# For the SPI protocol description, refer to 
# https://www.waveshare.net/w/upload/1/18/IT8951_D_V0.2.4.3_20170728.pdf and
# https://v4.cecdn.yun300.cn/100001_1909185148/IT8951_I80+ProgrammingGuide_16bits_20170904_v2.7_common_CXDX.pdf
class it8951:
    def __init__(self, spi: SPI, ncs: Pin):
        # There are 2 hardware SPI channels on the ESP32 and they can be mapped
        # to any pin, however, they are limited to 40MHz if not used on the 
        # default ones. The IT8951's maximum SPI speed is 24MHz either way, so 
        # we're good. The data must be clocked out MSB first at SPI Mode 0 or 3 
        # (0 used). At 24MHz a full refresh should take about <450ms
        self._spi = spi
        # Active-low chip select
        self._ncs = ncs
    
    def send_command(self, command: Command):
        """
        Sends a command to the IT8951.
        Args:
            command: Command to execute
        """
        try:
            self._ncs(0)
            txdata = ((SpiPreamble.COMMAND << 16) | command).to_bytes(4, 'big')
            self._spi.write(txdata)
        finally:
            self._ncs(1)
    
    def write_data(self, data: array):
        """
        Writes u16 words to the IT8951.
        Args: 
            data (array.array): A u16 array containing the data to be written
        Raises:
            ValueError: If the input data is not a u16 array with type code 'H'
        """
        if data.typecode != 'H':
            raise ValueError("Data must be u16[] with type code 'H'")
        if not data: return

        try:
            # Convert to big-endian format (MSByte first)
            data.byteswap()
            self._ncs(0)
            txdata = SpiPreamble.WRITE_DATA.to_bytes(2, 'big') + data.tobytes()
            self._spi.write(txdata)
        finally:
            self._ncs(1)
            
    def read_data(self, length: int) -> array:
        """
        Reads the specified number of 16bit words from the IT8951.
        Args:
            length: number of u16 elements to read
        """
        if length == 0: return array('H', [])
        try:
            # The first word returned from the controller is dummy:u16
            txdata = SpiPreamble.READ_DATA.to_bytes(2, 'big') + bytearray(length*2+2)
            rxdata = bytearray(len(txdata))
            self._ncs(0)
            self._spi.write_readinto(txdata, rxdata)
            # Take off the first 4 bytes (preampble and dummy words)
            u16_data = array('H', rxdata[4:])
            u16_data.byteswap()
            return u16_data
        finally:
            self._ncs(1)
            
    # Byte array data in little-endian format
    def write_reg(self, reg: Register, data: bytearray):
        self.send_command(Command.REG_WR)
        txdata = int.to_bytes(reg, 2, 'little').extend(data)
        self.write_data(txdata)
        
    def read_reg(self, reg: Register, length: int) -> bytearray:
        self.send_command(Command.REG_RD)
        self.write_data(int.to_bytes(reg, 2, 'little'))
        return self.read_data(length)

# The display can create images at 4bpp
# The display's full average update should take about 26.5mW

# The EPS waveform file should be stored in SPI flash





