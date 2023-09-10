from it8951 import it8951, Register

class epd:
    def __init__(tcon: it8951, self, width: int, height: int):
        # TODO: Continue
        tcon.write_reg(Register.BGVR)