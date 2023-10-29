import time
from machine import SPI, Pin
import pros3
import framebuf
from it8951 import *
from epd import epd

# Note: MicroPyhon does not support array objects well..
# Note: This code runs on Windows and MP as well 10/09/2023
# The display's full average update should take about 26.5mW

spi  = SPI(1, 24_000_000, sck=Pin(pros3.SPI_CLK), mosi=Pin(pros3.SPI_MOSI), miso=Pin(pros3.SPI_MISO), firstbit=SPI.MSB, polarity=0, phase=0, bits=8)
ncs  = Pin(34, Pin.OUT, value=1)
hrdy = Pin(9, Pin.IN, Pin.PULL_UP)
# Note that the waveform files in the IT8951 were created for the default vcom
# that's programmed in the chip, and not for the vcom that the display requires.
tcon = it8951(spi, ncs, hrdy, None)

# Clear the display to white
tcon.fill_rect(tcon.panel_area, DisplayMode.INIT, 0xF)

# Create a rainbow
#rect = Rectangle(0, 0, int(tcon.panel_area.width/16), tcon.panel_area.height)
#for i in range(0, 16):
#    rect.x = i*rect.width
#    tcon.fill_rect(rect, DisplayMode.GC16, i)

# Load a 4bpp grayscale image
tcon.load_bmp(0, 0, 'hand_grayscale.bmp')
tcon.display_area(tcon.panel_area, DisplayMode.GC16)

# Draw offset 32x32 rectangles
#img_info = ImageInfo(Endianness.LITTLE, ColorDepth.BPP_4BIT, RotateMode.ROTATE_0)
#image = [0]*1024
#for i in range(4):
#    rect = Rectangle(i, i*32, 32, 32)
#    pixels = it8951.pack_pixels(img_info, rect, image)
#    tcon.write_packed_pixels(img_info, rect, pixels)
#    tcon.display_area(rect, DisplayMode.GC16)

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

