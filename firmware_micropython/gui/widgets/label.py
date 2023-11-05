from widget import Widget
from rectangle import Point

class Label(Widget):
    def __init__(self, width, height, font):
        super().__init__(Point(0, 0), width, height)
        self.font = font
        self.text = ""
