import time
from machine import SPI, Pin
import pros3
from it8951 import *
from epd import epd

# Note: MicroPyhon does not support array objects well..
# Note: This code runs on Windows and MP as well 10/09/2023
# The display can create images at 4bpp
# The display's full average update should take about 26.5mW
# The EPD waveform file should be stored in SPI flash

spi  = SPI(1, 24_000_000, sck=Pin(pros3.SPI_CLK), mosi=Pin(pros3.SPI_MOSI), miso=Pin(pros3.SPI_MISO), firstbit=SPI.MSB, polarity=0, phase=0, bits=8)
ncs  = Pin(34, Pin.OUT, value=1)
hrdy = Pin(9, Pin.IN, Pin.PULL_UP)
tcon = it8951(spi, ncs, hrdy, -1580)

# Clear the display to white
tcon.fill_rect(tcon.panel_area, DisplayMode.INIT, 0xF)

rect = Rectangle(0, 0, int(tcon.panel_area.width/16), tcon.panel_area.height)
for i in range(0, 16):
    rect.x = i*rect.width
    tcon.fill_rect(rect, DisplayMode.GC16, i)

#image = [0xFF00, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x00ff]*32
#image = [0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000]*32
#image = [1]*256
#image = [128]*512
#rect = Rectangle(0, 0, 32, 32)
#img_info = ImageInfo(Endianness.LITTLE, ColorDepth.BPP_8BIT, RotateMode.ROTATE_0)
## TODO: The display of black pixels does not work with GC16...
#tcon.write_packed_pixels(img_info, rect, image)
#tcon.display_area(rect, DisplayMode.GC16)

#def do_connect():
#    import network
#    wlan = network.WLAN(network.STA_IF)
#    wlan.active(True)
#    if not wlan.isconnected():
#        print('connecting to network...')
#        wlan.connect('Talaria', 'Padkany9')
#        while not wlan.isconnected():
#            pass
#    print('network config:', wlan.ifconfig())

