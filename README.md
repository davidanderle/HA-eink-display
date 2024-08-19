# State of project (18 Aug 2024)
- LVGL can render any objects on the E-Ink screen through the IT8951
- HA ESPHome BLE Proxy ready to send data
- PC to ProS3 BLE data exchange works
- BLE Rx data written to SPLIFFS as a JSON
- LVGL can dinamically display the RX'd BLE data

# Introduction
This project contains the necessary BOM and software files to build a Home Assistant E-Ink Display I've been developed through the past years. This display is a thin, light-weight, battery-driven, [E Ink](https://www.eink.com/) [Home Assistant](https://www.home-assistant.io/) display that acts as a BLE peripheral (GATT server) and displays data from my Home Assitant instance. The displayed data at is
- Next 7 days of calendar events from 2 different calendars
- Our home's daily energy/water consumtion
- Real-time commuting time to 2 locations (like work)
- A QR code for the guest WiFi network
- Battery status
- Week number of the year
- House chores/extra TODOs
- Daily weather forecast
- Current weather
- Last refresh time

I started this project on 27 Aug 2023 and is a work in progress. My original attempt was to write the firmware in MicroPython using the LVGL graphics library, but alas after painful trials it turned out that LVGL's MicroPython port is not yet ready to to run on the ESP32-S3 chip. One legacy of this experiment is that now there is a MicroPython driver for the IT8951 chip to drive eink displays on one of the project's branches (TODO link).

# Hardware setup
1. Set the dip-switches into a 0b001 position (sw3 at ON position) to enable the SPI Slave communication. This is counter-intuitive as sw1 should've been bit0...
2. Ensure that the board is powered from a 5V line as the EPD PMIC needs this voltage. On the e-ink ICE driving board, I had to solder a wire on a resistor under the USB connector as the 5V line was not broken out on any of the pins...
![image](https://github.com/davidanderle/HA-eink-display/assets/17354704/3dca032c-fc01-4353-b139-fc69d722a0d5)
3. Download the [CP210x Universal Windows Driver](https://www.silabs.com/documents/public/software/CP210x_Universal_Windows_Driver.zip) and install it to allow you to upload code on the ProS3.
4. Connect the SPI_SCK, SPI_MOSI, SPI_MISO, SPI_CS and HRDY pins to the ICE board. 

# Hellow, world!
I am using a Visual Studio Code setup with PowerShell.
## Espressif setup through Platform-IO
1. Download the PlatformIO extension
2. Clone this repository. Once you open the folder, platformio should automatically recognise it as a project and pull in the required libraries and build tools. Now build the project
3. Put the ProS3 in download mode:
    - Press & hold the `BOOT` button
    - Press the `RST` button
    - Release the `BOOT` button
4. Upload the code on the ProS3
ESP32-S3 has a bug where the bootloader is not always entered/exitted on command, therefore the manual programming step is required...

## To upload the last binary without building
pio run -t nobuild -t upload

# Special character support
1. Convert ttf/wooff fonts [here](https://lvgl.io/tools/fontconverter) to LVGL format (see [ext_montserrat_14.c](https://github.com/davidanderle/eink_calendar/blob/espidf/src/ext_montserrat_14.c) as example)
2. Display uses [Montserrat](https://fonts.google.com/specimen/Montserrat) extended with [Noto emoji](https://fonts.google.com/noto/specimen/Noto+Emoji) as its default font. 
3. Supported emojies: 0x203C-0x3299 and 🤖🎃😀😁😂🤣😍🥰😘🥺🥚🐸👀🍆🥹😊🙂
4. The emoji set can easily be extended but it quickly eats up the flash
5. Check the encoding of characters on [UTF-8 tool](https://www.cogsci.ed.ac.uk/~richard/utf-8.cgi?input=1F970&mode=hex) 

# Test setup
Examine BLE and write to characteristics from [WebBluetooth](chrome://bluetooth-internals/#devices)
[Unity](https://github.com/ThrowTheSwitch/Unity) tests running on the ProS3

# HA Bluetooth proxy
[ESPHome Bluetooth proxy](https://esphome.io/projects/?type=bluetooth)

# BOM
- [ProS3](https://www.amazon.co.uk/gp/product/B09X22YBG7/ref=ewc_pr_img_2?smid=AGX9N6DGNRN2Q&psc=1) EPS32-S3 based WiFi+BLE+LiPo charger+PicoBlade to JST cable from [@UnexpectedMaker](https://github.com/UnexpectedMaker)'s [esp32s3](https://github.com/UnexpectedMaker/esp32s3) project, £26.99
- [375678 LiPo](https://www.aliexpress.com/item/1005004946019552.html?spm=a2g0o.cart.0.0.d80e38daNEjZz4&mp=1#nav-specification), 2500mAh, 3.7mm thick battery. (Positive and negative wires are swapped compared to the ProS3's LiPo charger!) £13.41
- [E-Ink VB3300-KCA](https://www.waveshare.com/product/displays/e-paper/epaper-1/10.3inch-e-paper-d.htm?___SID=U), flexible, 450ms full refresh, 4bpp, 10.3", 1872x1404 px display, £156.23+postage
- [ESP32-PICO-D4 WiFi BT donlge](https://www.aliexpress.com/item/1005006118961103.html), £5.58

# Reading the external SPI NOR flash from the ICE board
Using a J-Link Ultra+, download the SEGGER J-Flash SPI tool and go `Target > Read back > Entire chip`. Ensure that your SPI speed is sufficiently low to cope with the wiring's length. At `1MHz` it took me 34sec to download.
![image](https://github.com/davidanderle/HA-eink-display/assets/17354704/6486e221-4802-4124-b2e9-9e668b6178bf)

# UI design
I used (SquareLine Studio)[https://squareline.io/] to aid my UI design, but I soon ran into limitations. Therefore many elements were simply adjusted by hand. Some extra widgets were generated by AIs to suit my needs.

# References
ICE driving board
![image](https://github.com/davidanderle/HA-eink-display/assets/17354704/14772f9d-02dd-4990-bba2-ac562887a5ad)

