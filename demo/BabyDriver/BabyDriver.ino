// List Wifi nodes as you stroll.

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include <WiFi.h>

//OLED pins to ESP32 GPIOs via this connecthin:
//OLED_SDA -- GPIO4
//OLED_SCL -- GPIO15
//OLED_RST -- GPIO16

SSD1306  display(0x3c, 4, 15);

const unsigned int WIDTH = 128;
const unsigned int HEIGHT = 64;

void show_scan() {
  int num_networks = WiFi.scanNetworks();
  if (num_networks < 0) {
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(WIDTH/2,HEIGHT/2,"WiFi Not Found");
  }
  else {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    if (num_networks > 6) { num_networks=6; } // Not enough room to display everything, alas.
    for (int i = 0; i < num_networks; i++) {
      if (WiFi.encryptionType(i) == 7) { // Open network!!!
        display.fillRect(0,10*i,WIDTH,9);
        display.setColor(INVERSE);
      }
      display.drawString(0,10*i,WiFi.SSID(i));
      display.setColor(WHITE);
    }
  }
}

void setup() {
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  display.flipScreenVertically();
  display.displayOn();
  display.init();

  WiFi.begin();
}

void loop() {
  display.clear();
  show_scan();
  display.display();
  delay(500);  
}
