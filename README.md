ESP32 + OLED + LoRa module from TT GO
===
![SSD1306 UI demo](images/ssd1306-demo.jpg)

These $10 ESP32 modules from [aliexpress](https://www.aliexpress.com/item/2pcs-TTGO-LORA-SX1278-ESP32-0-96-OLED-16-Mt-bytes-128-Mt-bit-433Mhz-for/32833821668.html)
have a SSD1306 128x64 white OLED display and a SX1278 LoRa radio module.
They also include a LiPo battery charger and two buttons (one is wired to
reset, so it isn't as useful).

The official devkit is a 300 MB download, but if you have already setup
ESP32 on your Arduino IDE then you just need to install the libraries
for the OLED and LoRa (in the `libraries/` directory of this tree).

It would be nice to make the `Adafruit_SSD1306` library work with this
module so that it isn't necessary to install anything extra.  It looks like
they hacked up the Adafruit library to make their own.

Pinout
---
![TTGO ESP32 module pinout](images/esp32-pinout.jpg)


The ad copy says something about a touch screen:

  Using the IO port touch screen touch signal input,
  you need to add the 100nF pull-down capacitor at
  this pin!

There doesn't seem to be any discussion of it in the SDK, nor is it
described with pin needs to pull-down.



