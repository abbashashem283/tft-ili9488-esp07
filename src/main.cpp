#include <Arduino.h>

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Pins
#define CS_PIN 16      // Chip select for XPT2046
#define TIRQ_PIN -1    // Optional, can be -1 if unused

// Touchscreen object
XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);

TFT_eSPI tft;

bool red = true;

void setup() {
  delay(10000);
  Serial.begin(9600);
  //SPI.begin();  // GPIO14=SCK, GPIO13=MOSI, GPIO12=MISO
  ts.begin();
  ts.setRotation(3); // Adjust rotation to match your TFT
  Serial.println("XPT2046 Touch test");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_RED); // white text, red background
  tft.setTextSize(3);
  tft.setCursor(360, 300);
  tft.print("Hello");
}

void loop() {
  if(ts.touched()){
    
    TS_Point p = ts.getPoint();
    uint16_t x = (p.x * 480) / 4095 ;
    uint16_t y = (p.y * 320) / 4095 ;
    Serial.print("X: "); Serial.print(x);
    Serial.print(" Y: "); Serial.print(y);
    Serial.print(" Z: "); Serial.println(p.z);
  }
}
