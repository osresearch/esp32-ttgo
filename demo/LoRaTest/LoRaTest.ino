// Basic LoRa packet tester.

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include <LoRa.h>

//OLED pins to ESP32 GPIOs via this connecthin:
//OLED_SDA -- GPIO4
//OLED_SCL -- GPIO15
//OLED_RST -- GPIO16

SSD1306  display(0x3c, 4, 15);

const unsigned int WIDTH = 128;
const unsigned int HEIGHT = 64;


void processPacket(int size) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0,0,"LoRa Packet Received:");
  char buf[size];
  for (int i = 0; i < size; i++) {
    buf[i] = LoRa.read();
  }
  buf[size-1] = 0;
  display.drawString(0,10, buf);
  display.drawString(20,54,"Push to send packet ->");
  display.display();
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

  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0,0,"Waiting for LoRa packet.");  
  display.drawString(20,54,"Push to send packet ->");
  display.display();

  pinMode(0,INPUT_PULLUP);
  
  //LoRa.setPins(10,9,2);
  LoRa.begin(433e6);
  LoRa.onReceive(processPacket);
  LoRa.receive();
}

int packet_num = 0;
bool bdown = false;

void loop() {
  delay(50);
  if (!bdown && digitalRead(0) == LOW) {
    bdown = true;
    display.drawString(0,20,"Sendng LoRa packet!");
    display.display();

    String hello = "Hello! Packet #"+String(packet_num++);
    LoRa.beginPacket();
    LoRa.write((const uint8_t*)hello.c_str(),hello.length());
    LoRa.write(0);
    LoRa.endPacket();
  } else {
    if (digitalRead(0) == HIGH) { bdown = false; }
  }
  int sz = LoRa.parsePacket();
  if (sz > 0) {
    processPacket(sz);
  }
}
