# eink_calendar
I started this project on 27 Aug 2023 and is a work in progress
ESP32-S3 consumes 5uA in deep sleep

# Goal
A thin, light-weight, long battery life E-Ink-based calendar that syncs its content to 2 gmail calendars.

The embedded UI design is likely to be through 

# TODO
- Charge current to set to around 270mA-350mA on the ProS3 by Rprog=3.3k. Should be OK for the 375678 LiPo
- Implement MicroPython on ProS3
- Implement LVGL from SquarlineStudio on ProS3 to render the UI
- Connect Google API from uPython
- Display data
- Compile the uPython code to .mpy using `mpy-cross` to accelerate execution and occupy less flash
- investigate webrepl https://micropython.org/webrepl/ to update files over-the-air

# Hellow, world!
I could not make serial communication work from WSL2 on my laptop, therefore I am using VSCode + PowerShell terminal.
1. Download the latest [esptool](https://github.com/espressif/esptool/releases/tag/v4.6.2) to program the ESP32-S3 through its ROM bootloader
2. Download the latest [micropython .bin](https://micropython.org/download/UM_PROS3/)
3. Put the ProS3 in download mode:
   - Press & hold the `BOOT` button
   - Press the `RST` button
   - Release the `BOOT` button
4. Load the MicroPython binary on the ProS3 (replace `x` with your COM port) from CMD:
```cmd
esptool --chip esp32s3 --port COMx erase_flash
esptool --chip esp32s3 --port COMx write_flash -z 0x0 micropython_v1.20.0.bin
```
5. Download and install [Python3](https://www.python.org/downloads/)
6. Install [mpremote](https://pypi.org/project/mpremote/) from PowerShell
```PowerShell
py -m pip install mpremote
py -m mpremote version
```
7. Copy over the [pros3.py](https://github.com/UnexpectedMaker/esp32s3/blob/main/code/micropython/helper%20libraries/pros3/pros3.py) helper library and the [example.py](https://github.com/UnexpectedMaker/esp32s3/blob/main/code/micropython/helper%20libraries/pros3/example.py) files and run the example code
```
py -m mpremote cp pros3.py :pros3.py + cp example.py :example.py + run example.py

Hello from ProS3!
------------------

Memory Info - gc.mem_free()
---------------------------
8191392 Bytes

Flash - os.statvfs('/')
---------------------------
Size: 14680064 Bytes
Free: 14659584 Bytes

Pixel Time!
```

# Test setup
1. Ensure that pip is in the PATH:
```
$Env:Path += ';C:\<path-to-pythonXXX>\Scripts'
```
2. Install unittest requirements
```
pip install parameterized
```
3. Configure VSCode's Test Explorer:
Create (or add to) your .vscode/settings.json and paste these data/value pairs:
```
{
    "python.testing.unittestArgs": [
        "-v",
        "-s",
        "./firmware/tests",
        "-p",
        "test_*.py"
    ],
    "python.testing.pytestEnabled": false,
    "python.testing.unittestEnabled": true
}
```
If done correctly, this is what you should roughly see when pressing the conical flash button on the LHS of VSCode's primary side bar:
![image](https://github.com/davidanderle/eink_calendar/assets/17354704/4780c91f-caff-4769-8716-3f894de77eec)

# Hardware setup
1. Set the dip-switches into a 0b001 position (sw3 at ON position) to enable the SPI Slave communication. This is counter-intuitive as sw1 should've been bit0...
2. Ensure that the board is powered from a 5V line as the EPD PMIC needs this voltage. On the e-ink ICE driving board, I had to solder a wire on a resistor under the USB connector as the 5V line was not broken out on any of the pins...
![image](https://github.com/davidanderle/eink_calendar/assets/17354704/3dca032c-fc01-4353-b139-fc69d722a0d5)

# BOM
- [ProS3](https://www.amazon.co.uk/gp/product/B09X22YBG7/ref=ewc_pr_img_2?smid=AGX9N6DGNRN2Q&psc=1) EPS32-S3 based WiFi+BLE+LiPo charger+PicoBlade to JST cable from [@UnexpectedMaker](https://github.com/UnexpectedMaker)'s [esp32s3](https://github.com/UnexpectedMaker/esp32s3) project, £26.99
- [375678 LiPo](https://www.aliexpress.com/item/1005004946019552.html?spm=a2g0o.cart.0.0.d80e38daNEjZz4&mp=1#nav-specification), 2500mAh, 3.7mm thick battery. (Positive and negative wires are swapped compared to the ProS3's LiPo charger!) £13.41
- [E-Ink VB3300-KCA](https://www.waveshare.com/product/displays/e-paper/epaper-1/10.3inch-e-paper-d.htm?___SID=U), flexible, 450ms full refresh, 4bpp, 10.3", 1872x1404 px display, £156.23+postage


# Reading the external SPI NOR flash from the ICE board
Using a J-Link Ultra+, download the SEGGER J-Flash SPI tool and go `Target > Read back > Entire chip`. Ensure that your SPI speed is sufficiently low to cope with the wiring's length. At `1MHz` it took me 34sec to download.
![image](https://github.com/davidanderle/eink_calendar/assets/17354704/6486e221-4802-4124-b2e9-9e668b6178bf)

# GUI design
Download nano-gui on the ProS3
```
py -m mpremote mip install "github:peterhinch/micropython-nano-gui"
```

### TODO
Note that the build only succeeds for BOARD=GENERIC_S3_SPIRAM, following the patches recommended [here](https://github.com/lvgl/lv_binding_micropython/issues/227#issuecomment-1596203164). Check if this works with the ProS3 and if not, use this build setup to modify the relevant files to enable the compilation for the ProS3

# References

ICE driving board
![image](https://github.com/davidanderle/eink_calendar/assets/17354704/14772f9d-02dd-4990-bba2-ac562887a5ad)

MicroPython API
https://docs.micropython.org/en/latest/esp32/quickref.html#

mpremote control
https://docs.micropython.org/en/latest/reference/mpremote.html

https://www.waveshare.com/wiki/9.7inch_e-Paper_HAT

https://github.com/speedyg0nz/MagInkCal

https://micropython.org/download/?mcu=esp32s3

https://imageresizer.com/

https://www.lcsc.com/product-detail/Battery-Management-ICs_TOPPOWER-Nanjing-Extension-Microelectronics-TP4065-4-2V-SOT25-R_C113540.html

