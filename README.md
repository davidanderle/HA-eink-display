# Introduction
I started this project on 27 Aug 2023 and is a work in progress. My original attempt was to write the firmware in MicroPython using the LVGL graphics library, but alas after painful trials it turned out that LVGL's MicroPython port is not yet ready to to run on the ESP-S3 chip. One legacy of this experiment is that now there is a MicroPython driver for the IT8951 chip to drive eink displays.

# Goal
A thin, light-weight, long battery life E-Ink-based calendar that syncs its content to 2 gmail calendars. The display would show 7 days (rolling) of events in a portrait orientation, along with the estimated time to work for the 2 google account holders. It would also have a quick weather overview and a battery life indicator.

# Hellow, world!

# Test setup

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

# References

ICE driving board
![image](https://github.com/davidanderle/eink_calendar/assets/17354704/14772f9d-02dd-4990-bba2-ac562887a5ad)

