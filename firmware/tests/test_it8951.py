import unittest
from array import array
from parameterized import parameterized
from unittest.mock import Mock, patch
import sys
# Ensure that the parent directory is visible from this module
sys.path.append('..')
from it8951 import it8951, Command

def spi_write(data: bytearray):
    #formatted_bytes = '-'.join([f'0x{byte:02X}' for byte in data])
    #print(f'SPI MOSI: {formatted_bytes}')
    txed_bytes.extend(data)

def gpio_set_value(val: int):
    #print("SPI nCS", val)
    pass

mock_spi = Mock()
mock_spi.write.side_effect = spi_write

mock_ncs = Mock()
mock_ncs.side_effect = gpio_set_value

# Pretend that the HRDY is high (IT8951 ready to accept commands)
mock_hrdy = Mock()
mock_hrdy.value.return_value = 1

txed_bytes = bytearray()

class test_it8951(unittest.TestCase):
    print("==[Running it8951 tests]==")
    @parameterized.expand([
        (Command.SYS_RUN,      [0x60, 0x00, 0x00, 0x01]),
        (Command.STANDBY,      [0x60, 0x00, 0x00, 0x02]),
        (Command.CMD_VCOM,     [0x60, 0x00, 0x00, 0x39]),
        (Command.GET_DEV_INFO, [0x60, 0x00, 0x03, 0x02]),
    ])
    def test_send_command(self, command, expected_bytes):
        tcon = it8951(mock_spi, mock_ncs, mock_hrdy)
        tcon.send_command(command)
        self.assertEqual(txed_bytes, bytes(expected_bytes))
    
    @parameterized.expand([
        ([0x0000, 0x0203, 0x4567], [0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x45, 0x67]),
        ([0x7FFF],                 [0x00, 0x00, 0x7F, 0xFF]),
        ([0xFFFF],                 [0x00, 0x00, 0xFF, 0xFF]),
        ([],                       [])
    ])
    def test_write_data(self, data, expected_bytes):
        tcon = it8951(mock_spi, mock_ncs, mock_hrdy)
        tcon.write_data(array('H', data))
        self.assertEqual(txed_bytes, bytes(expected_bytes))
    
    @parameterized.expand([
        (0, [],                               []),
        (1, [0x1234],                         [0x10,0,0,0,0,0]),
        (4, [0x0000, 0x7FFF, 0x8000, 0xFFFF], [0x10,0,0,0,0,0,0,0,0,0,0,0])
    ])
    def test_read_data(self, len, expected_words, expected_tx):
        def spi_write_readinto(txdata: bytearray, rxdata: bytearray):
            u16_array = array('H', [0,0]+expected_words)
            u16_array.byteswap()
            rxdata[:] = u16_array.tobytes()
            txed_bytes.extend(txdata)
            #formatted_bytes = '-'.join([f'0x{byte:02X}' for byte in rxdata])
            #print(f'SPI MISO: {formatted_bytes}')
            #formatted_bytes = '-'.join([f'0x{byte:02X}' for byte in txdata])
            #print(f'SPI MOSI: {formatted_bytes}')

        mock_spi.write_readinto = spi_write_readinto
        tcon = it8951(mock_spi, mock_ncs, mock_hrdy)
        rxed_words = tcon.read_data(len)
        self.assertEqual(rxed_words, expected_words)
        self.assertEqual(txed_bytes, bytes(expected_tx))
    
    def setUp(self) -> None:
        # Ensure that the buffer is clear before each test
        txed_bytes.clear()
        return super().setUp()