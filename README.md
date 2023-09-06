# eink_calendar
I started this project on 27 Aug 2023 and is a work in progress as of 28 Aug 2023.
ESP-S3 consumes 5uA in deep sleep

# Goal
A thin, light-weigth, long battery life E-Ink-based calendar that syncs its contect to 2 gmail calendars.

The embedded UI design is likely to be through 

# TODO
- Charge current to set to around 270mA-350mA on the ProS3 by Rprog=3.3k. Should be OK for the 375678 LiPo
- Implement MicroPython on ProS3
- Implement LVGL from SquarlineStudio on ProS3 to render the UI
- Connect Google API from uPython
- Display data
- Compile the uPython code to .mpy using `mpy-cross` to accelerate execution and occupy less flash
- investigate webrepl https://micropython.org/webrepl/ to update files over-the-air

I could not make serial communication work from WSL2 on my laptop, therefore I am using a slightly more convoluted way to communicate with the board.
My setup is in VSCode + PowerShell terminal.
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
7. Copy over the [pros3.py](https://github.com/UnexpectedMaker/esp32s3/blob/main/code/micropython/helper%20libraries/pros3/pros3.py) helper library and the [example.py](https://github.com/UnexpectedMaker/esp32s3/blob/main/code/micropython/helper%20libraries/pros3/example.py) files
```
py -m mpremote cp pros3.py :pros3.py
py -m mpremote cp example.py :example.py
```
8. Run the code and you should see:
```
PS C:\mypath> py -m mpremote run example.py    

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

To open a REPL terminal communication with the ProS3 `py -m mpremote`

# Hardware setup
1. Set the dip-switches into a 0b001 position (sw0 at ON position) to enable the SPI Slave communication

# BOM
- [ProS3](https://www.amazon.co.uk/gp/product/B09X22YBG7/ref=ewc_pr_img_2?smid=AGX9N6DGNRN2Q&psc=1) EPS32-S3 based WiFi+BLE+LiPo charger+PicoBlade to JST cable from [@UnexpectedMaker](https://github.com/UnexpectedMaker)'s [esp32s3](https://github.com/UnexpectedMaker/esp32s3) project, £26.99
- [375678 LiPo](https://www.aliexpress.com/item/1005004946019552.html?spm=a2g0o.cart.0.0.d80e38daNEjZz4&mp=1#nav-specification), 2500mAh, 3.7mm thick battery £13.41
- [E-Ink VB3300-KCA](https://www.waveshare.com/product/displays/e-paper/epaper-1/10.3inch-e-paper-d.htm?___SID=U), flexible, 450ms full refresh, 4bpp, 10.3", 1872x1404 px display, £156.23+postage

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

