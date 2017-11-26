ESP32 + OLED + LoRa module from TT GO
===
![SSD1306 UI demo](images/ssd1306-demo.jpg)

These $10 ESP32 modules from various aliexpress sellers [(one example)](https://www.aliexpress.com/item/2pcs-TTGO-LORA-SX1278-ESP32-0-96-OLED-16-Mt-bytes-128-Mt-bit-433Mhz-for/32833821668.html)
have a SSD1306 128x64 white OLED display and a SX1278 LoRa radio module.
They also include a LiPo battery charger and two buttons (one is wired to
reset, so it isn't as useful).  There is a slightly more [expensive version
with a blue OLED](https://www.aliexpress.com/item/2pcs-of-868MHz-915MHz-SX1276-ESP32-LoRa-0-96-Inch-Blue-OLED-Display-Bluetooth-WIFI-Kit/32840618066.html)
with a 900 Mhz radio.

Libraries
===
The official devkit is a 300 MB download, but if you have already setup
ESP32 on your Arduino IDE then you just need to install the libraries
for the OLED and LoRa (in the [`libraries/`](libraries/) directory of
this tree).  From the Arduino IDE select
`Sketch`-`Include Libaries`-`Add .zip library`
and select the libraries zips.

It would be nice to make the `Adafruit_SSD1306` library work with this
module so that it isn't necessary to install anything extra.  It looks like
they hacked up the Adafruit library to make their own.

The LoRa library might not be as hacked up.  Haven't investigated it yet.

The other libraries should be pre-installed, such as the `Preferences`
to access the ESP32 non-volatile storage, and the `Wire` library for
i2c communication.

Pinout
===
The 400 MHz version with the white OLED:
![TTGO ESP32 module pinout](images/esp32-pinout.jpg)

The 900 MHz version with the blue OLED is almost exactly backwards.
Be sure to note that the components are turned around if you plan to try both
modules.
![TTGO ESP32 module pinout](images/esp32-pinout2.jpg)


The ad copy says something about a touch screen:

    Using the IO port touch screen touch signal input,
    you need to add the 100nF pull-down capacitor at
    this pin!

There doesn't seem to be any discussion of it in the SDK, nor is it
described with pin needs to pull-down.

OLED notes
===

The reset pin on the OLED needs to be controlled by the sketch. It does
not seem to be included in the library.  Pin 16 is the reset line and
it should be brought low for a few ms, then held high the entire time
the screen is to remain on.

	#include <Wire.h>
	#include <SSD1306.h>
	#include <OLEDDisplayUi.h>

	//OLED pins to ESP32 GPIOs via this connecthin:
	//OLED_SDA -- GPIO4
	//OLED_SCL -- GPIO15
	//OLED_RST -- GPIO16

	SSD1306 display(0x3c, 4, 15); // i2c address, SDA pin, SCL pin
	OLEDDisplayUi ui( &display );

	void setup()
	{
		pinMode(16,OUTPUT);
		digitalWrite(16, LOW); // GPIO16 low to reset OLED
		delay(50); 
		digitalWrite(16, HIGH); // GPIO16 must be high to turn on OLED
		// ....
	}
