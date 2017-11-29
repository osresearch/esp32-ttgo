
#include <Esp.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "OLEDDisplayUi.h"
#include <WiFi.h>

//OLED pins to ESP32 GPIOs via this connecthin:
//OLED_SDA -- GPIO4
//OLED_SCL -- GPIO15
//OLED_RST -- GPIO16

SSD1306  display(0x3c, 4, 15);

OLEDDisplayUi ui     ( &display );


void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String chipRevStr = String("Chip revision: 0x")+String(ESP.getChipRevision(),HEX);
  String cpuFreqStr = String("Frequency: ")+String(ESP.getCpuFreqMHz()) + String(" MHz");
  String sdkVerStr = String("SDK: ")+String(ESP.getSdkVersion());
  uint64_t macAddr = ESP.getEfuseMac();
  uint8_t* macAddrArr = (uint8_t*)&macAddr;
  String macAddrStr = String("MAC: ");
  for (int i = 5; i >= 0; i--) {
    macAddrStr += String(macAddrArr[i],HEX);
    if (i > 0) macAddrStr += ":";
  }
    
  // Frame 1: ESP info
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x,y,"ESP32");

  display->setFont(ArialMT_Plain_10);
  display->drawString(x,y+16,chipRevStr);
  display->drawString(x,y+26,cpuFreqStr);
  display->drawString(x,y+36,sdkVerStr);
  display->drawString(x,y+46,macAddrStr);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String szStr = String("Flash size: ") + String(ESP.getFlashChipSize());
  String spStr = String("Flash speed: ") + String(ESP.getFlashChipSpeed());
  
  // Frame 2: Flash info
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x,y,"Flash");

  display->setFont(ArialMT_Plain_10);
  display->drawString(x,y+16,szStr);
  display->drawString(x,y+26,spStr);
}

int num_networks = 0;
unsigned long last_net_ping = 0;

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Frame 3: Available networks
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x,y,"Nearby Networks");
  if (num_networks < 0) {
    display->drawString(x,y+16,"WiFi Not Found");
  } else if (num_networks == 0) {
    display->drawString(x,y+16,"No Networks Found");
  } else {
    display->setFont(ArialMT_Plain_10);
    if (num_networks > 4) { num_networks=4; } // Not enough room to display everything, alas.
    for (int i = 0; i < num_networks; i++) {
      if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) { // Open network!!!
        display->fillRect(x,16+y+10*i,128,9);
        display->setColor(INVERSE);
      }
      display->drawString(x,16+y+10*i,WiFi.SSID(i));
      display->setColor(WHITE);
    }
  }
}


// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3 };

// how many frames are there?
int frameCount = 3;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = {  };
int overlaysCount = 0;

void setup() {
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  
  Serial.begin(115200);
  Serial.println();
  Serial.println();

	// The ESP is capable of rendering 60fps in 80Mhz mode
	// but that won't give you much time for anything else
	// run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(60);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();
  WiFi.begin();
  num_networks = WiFi.scanNetworks();
}


void loop() {
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    if (ui.getUiState()->frameState == FIXED && (millis() - last_net_ping) > 5000) {
      num_networks = WiFi.scanNetworks();
      last_net_ping = millis();
    }
    else {
      delay(remainingTimeBudget);
    }
  }
}
