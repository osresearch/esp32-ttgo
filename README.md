ESP32 + OLED + LoRa module from TT GO
===

These $10 ESP32 modules from [aliexpress](https://www.aliexpress.com/item/2pcs-TTGO-LORA-SX1278-ESP32-0-96-OLED-16-Mt-bytes-128-Mt-bit-433Mhz-for/32833821668.html)
have a SSD1306 128x64 white OLED display and a SX1278 LoRa radio module.
They also include a LiPo battery charger and two buttons.

The official devkit is a 300 MB download, but if you have already setup
ESP32 on your Arduino IDE then you just need to install the libraries
for the OLED and LoRa (in the `libraries/` directory of this tree).

It would be nice to make the `Adafruit_SSD1306` library work with this
module so that it isn't necessary to install anything extra.

Pinout
---
![TTGO ESP32 module pinout](images/esp32-pinout.jpg)


