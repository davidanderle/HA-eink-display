import struct
import unittest
from parameterized import parameterized
from unittest.mock import Mock, patch
import sys
import os
# Ensure that the parent directory is visible from this module
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, project_root)
from it8951 import it8951, Command

class test_it8951(unittest.TestCase):
    print("==[Running it8951 tests]==")
    @parameterized.expand([
        (Command.SYS_RUN,      [0x60, 0x00, 0x00, 0x01]),
        (Command.STANDBY,      [0x60, 0x00, 0x00, 0x02]),
        (Command.CMD_VCOM,     [0x60, 0x00, 0x00, 0x39]),
        (Command.GET_DEV_INFO, [0x60, 0x00, 0x03, 0x02]),
    ])
    def test_send_command(self, command: Command, expected_bytes: list):
        tcon = it8951(self.mock_spi, self.mock_ncs, self.mock_hrdy)
        self.assertEqual(self.ncs, 1)
        tcon._send_command(command)
        self.assertEqual(self.ncs, 1)
        self.assertEqual(self.txed_bytes, expected_bytes)
    
    @parameterized.expand([
        ([0x0000, 0x0203, 0x4567], [0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x45, 0x67]),
        ([0x7FFF],                 [0x00, 0x00, 0x7F, 0xFF]),
        ([0xFFFF],                 [0x00, 0x00, 0xFF, 0xFF]),
        ([],                       [])
    ])
    def test_write_data(self, data: list, expected_bytes: list):
        tcon = it8951(self.mock_spi, self.mock_ncs, self.mock_hrdy)
        self.assertEqual(self.ncs, 1)
        tcon._write_data(data)
        self.assertEqual(self.ncs, 1)
        self.assertEqual(self.txed_bytes, expected_bytes)
    
    @parameterized.expand([
        (0, [],                               []),
        (1, [0x1234],                         [0x10,0,0,0,0,0]),
        (4, [0x0000, 0x7FFF, 0x8000, 0xFFFF], [0x10,0,0,0,0,0,0,0,0,0,0,0])
    ])
    def test_read_data(self, len: int, expected_words: list, expected_tx: list):
        expected_bytes = b''.join(struct.pack('>H', word) for word in [0, 0] + expected_words)
        def spi_read(nbytes: int, txdata: int) -> bytearray:
            self.txed_bytes.extend([txdata])
            ret = expected_bytes[self.rxcounter:self.rxcounter+2]
            self.rxcounter += nbytes
            return ret

        self.mock_spi.read = spi_read
        tcon = it8951(self.mock_spi, self.mock_ncs, self.mock_hrdy)
        self.assertEqual(self.ncs, 1)
        rxed_words = tcon._read_data(len)
        self.assertEqual(self.ncs, 1)
        self.assertEqual(rxed_words, expected_words)
        self.assertEqual(self.txed_bytes, expected_tx)

    @parameterized.expand([
        (-1580, False, [0x60, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x01, 0x06, 0x2C]),
        (-1580, True,  [0x60, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x02, 0x06, 0x2C]),
        (1800,  False, []),
        (0,     False, [])
    ])
    def test_set_vcom(self, vcom_mV: int, write_to_flash: bool, expected_tx: list):
        tcon = it8951(self.mock_spi, self.mock_ncs, self.mock_hrdy)
        self.assertEqual(self.ncs, 1)
        if vcom_mV >= 0:
            with self.assertRaises(Exception): tcon.set_vcom(vcom_mV, write_to_flash)
        else:
            tcon.set_vcom(vcom_mV, write_to_flash)
        self.assertEqual(self.ncs, 1)
        self.assertEqual(self.txed_bytes, expected_tx)
    
    # This method is called before any tests are run 
    @classmethod
    def setUpClass(cls):
        def spi_write(data: bytearray):
            assert cls.ncs == 0, "nCS must be set low during SPI TxR"
            cls.txed_bytes.extend(data)

        def gpio_set_ncs(val: int):
            cls.ncs = val

        cls.mock_spi = Mock()
        cls.mock_spi.write.side_effect = spi_write

        cls.mock_ncs = Mock()
        cls.mock_ncs.side_effect = gpio_set_ncs

        # Pretend that the HRDY is high (IT8951 ready to accept commands)
        cls.mock_hrdy = Mock()
        cls.mock_hrdy.value.return_value = 1

        cls.txed_bytes = []
        cls.ncs = 1
        
    # This method is called before each test. Ensure that the buffer is clear before each test
    def setUp(self) -> None:
        self.txed_bytes.clear()
        self.rxcounter = 0
        return super().setUp()