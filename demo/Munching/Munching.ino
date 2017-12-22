// Classic Munching Squares effect.

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`

//OLED pins to ESP32 GPIOs via this connecthin:
//OLED_SDA -- GPIO4
//OLED_SCL -- GPIO15
//OLED_RST -- GPIO16

SSD1306  display(0x3c, 4, 15);

const unsigned int WIDTH = 128;
const unsigned int HEIGHT = 64;

// 128x64 display
unsigned char image[WIDTH*HEIGHT/8];

void munching(unsigned char t) {
  for (int i = 0; i < WIDTH/8; i++) {
    for (int j = 0; j < HEIGHT; j++) {
      const unsigned int idx = j*(WIDTH/8) + i;
      unsigned char c = 0x0;
      for (int k = 0; k < 8; k++) {
        const unsigned int y = j;
        const unsigned int x = (i*8)+k;
        if ((x^y) < t) {
          c |= (1<<(7-k));
        }       
      }
      image[idx]=c;
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
}

unsigned char t = 0;

void loop() {
  munching(t);
  t++;
  if (t > 128) t = 0;
  display.clear();
  display.drawXbm(0,0,128,64,(const char*)image);
  // Swap over to get an interleaved munching squares :)
  //display.drawFastImage(0,0,128,64,(const char*)image);
  display.display();
  delay(2);  
}
