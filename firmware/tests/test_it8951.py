import unittest
from unittest.mock import Mock, patch
import sys
# Ensure that the parent directory is visible from this module
sys.path.append('..')
from it8951 import it8951, Command

def spi_write(data: bytearray):
    formatted_bytes = '-'.join([f'0x{byte:02X}' for byte in data])
    print(f'SPI MOSI: {formatted_bytes}')
    txed_bytes.extend(data)

def gpio_set_value(val: int):
    print("SPI nCS", val)

mock_spi = Mock()
mock_spi.write.side_effect = spi_write

mock_ncs = Mock()
mock_ncs.side_effect = gpio_set_value

txed_bytes = bytearray()

class test_it8951(unittest.TestCase):
    def test_send_command(self):
        obj = it8951(mock_spi, mock_ncs)
        
        obj.send_command(Command.SYS_RUN)
        self.assertEqual(txed_bytes, bytes([0x60, 0x00, 0x00, 0x01]))
    
    #def test_write_data(self):
    #    obj
    
    def setUp(self) -> None:
        # Ensure that the buffer is clear before each test
        txed_bytes = bytearray()
        return super().setUp()