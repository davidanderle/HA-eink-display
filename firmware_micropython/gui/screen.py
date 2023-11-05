from gui import *
from gui.widgets import *
from rectangle import Rectangle
from config import BIT_FORMAT, BIT_DEPTH
import framebuf

class Screen(framebuf.FrameBuffer):
    def __init__(self, width: int, height: int):
        """
        This class is a frame buffer that handles the rendering and the drawing
        on the physical display.
        Args:
            width: Width of the display
            height: Height of the display
        """
        self.widgets = []
        self.width = width
        self.height = height
        super().__init__(bytearray(width*height//BIT_DEPTH), width, height, BIT_FORMAT)
    
    def clear(self):
        self.fill(0)

    def add(self, widget: Widget):
        self.widgets.append(widget)
        rect = Rectangle(widget.location, widget.width, widget.height)
        self._update(widget)
        self.display_flush(rect, )

    def display_flush(area: Rectangle, buff: bytearray):
        """
        User-defined function to flush the changed pixels to the display
        """
        pass

    def _update(self, widget) -> bytearray:
        """
        Updates display's frame buffer with the widget
        """
        for row in range(widget.height):
            for col in range(widget.width):
                pix = widget.get_pixel(col, row)
                if pix is not None:
                    self.pixel(widget.location.x + col, widget.location.y + row, pix)
        return 
