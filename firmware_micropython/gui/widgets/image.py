from widget import Widget
from rectangle import Point

class Image(Widget):
    def __init__(self, path: str):
        self.path = path
        with open(path, 'rb') as img:
            # TOOD: Load the image
            width = 0
            height = 0
        super().__init__(Point(0, 0), width, height)
    