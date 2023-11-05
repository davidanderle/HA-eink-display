from config import BIT_DEPTH, BIT_FORMAT
from rectangle import Point
import framebuf

class Widget(framebuf.FrameBuffer):
    def __init__(self, location: Point, width: int, height: int):
        self.width = width
        self.height = height
        self.location = location
        super().__init__(bytearray(width*height//BIT_DEPTH), width, height, BIT_FORMAT)


    def get_pix(self, x: int, y: int) -> int:
        # TODO:
        return 0
