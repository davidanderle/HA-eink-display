from machine import SPI, Pin
from array import array
import pros3
from it8951 import it8951, Command, Register
from epd import epd

# Note: MicroPyhon does not support array objects well..
# Note: This code runs on Windows and MP as well 10/09/2023
# The display can create images at 4bpp
# The display's full average update should take about 26.5mW
# The EPD waveform file should be stored in SPI flash

spi  = SPI(1, 24_000_000, sck=Pin(pros3.SPI_CLK), mosi=Pin(pros3.SPI_MOSI), miso=Pin(pros3.SPI_MISO), firstbit=SPI.MSB, polarity=0, phase=0, bits=8)
ncs  = Pin(34, Pin.OUT, value=1)
hrdy = Pin(9, Pin.IN, Pin.PULL_UP)
tcon = it8951(spi, ncs, hrdy)

epd_ctrl = epd(tcon, 1872, 1404, -1580)

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

