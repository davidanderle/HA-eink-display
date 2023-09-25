from machine import SPI, Pin
from array import array
import pros3
from it8951 import it8951, Command, Register

# Note: MicroPyhon does not support array objects well..
# Note: as it, it runs on Windows and MP as well! 10/09/2023

# The display can create images at 4bpp
# The display's full average update should take about 26.5mW

# The EPS waveform file should be stored in SPI flash

# TODO: The channel id may not be 1
# TODO: 16bit frame-size may work as well
spi = SPI(2, 24_000_000, sck=Pin(pros3.SPI_CLK), mosi=Pin(pros3.SPI_MOSI), miso=Pin(pros3.SPI_MISO), firstbit=SPI.MSB, polarity=0, phase=0, bits=8)
ncs = Pin(34, Pin.OUT, value=1)
hrdy = Pin(9, Pin.IN, Pin.PULL_UP)
tcon = it8951(spi, ncs, hrdy)
device_info = tcon.get_device_info()
#tcon.write_data([0x1234])
#tcon.read_data(2)
#tcon.send_command(Command.SET_VCOM)
#tcon.write_reg(Register.MCSR, [0x1234])
#mcsr = tcon.read_reg(Register.MCSR, 1)
#print(mcsr)

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

