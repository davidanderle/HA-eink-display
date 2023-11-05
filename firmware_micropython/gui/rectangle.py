from collections import namedtuple

Point = namedtuple('Point', 'x', 'y')

class Rectangle:
    def __init__(self, point: Point, w: int, h: int):
        self.point  = point
        self.width  = w
        self.height = h

    def area(self) -> int:
        return self.width*self.height
        
    def to_list(self) -> list:
        return [self.point.x, self.point.y, self.width, self.height]
    
    def is_contained_within(self, rect: 'Rectangle') -> bool:
        return (
            self.point.x >= rect.point.x and
            self.point.y >= rect.point.y and
            self.point.x + self.width <= rect.point.x + rect.width and
            self.point.y + self.height <= rect.point.x + rect.height
        )

    def __str__(self) -> str:
        return f"(x,y): ({self.point.x},{self.point.y})\n" + \
               f"(w,h): ({self.width},{self.height})\n" + \
               f"Area: {self.area()}"