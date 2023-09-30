from it8951 import it8951, Register

class epd:
    def __init__(self, tcon: it8951, width: int, height: int, vcom_mV: int):
        self.device_info = tcon.get_device_info()
        print(self.device_info)

        rxvcom_mV = tcon.get_vcom()
        print(f"Current VCOM = {rxvcom_mV/1000}")

        if vcom_mV != rxvcom_mV:
            print(f"Settig VCOM to new value: {vcom_mV/1000}")
            tcon.set_vcom(vcom_mV)
            rxvcom_mV = tcon.get_vcom()
            print("Success" if tcon.get_vcom() == vcom_mV else "Failed")