#include <Arduino.h>

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Pins
#define CS_PIN 16   // Chip select for XPT2046
#define TIRQ_PIN -1 // Optional, can be -1 if unused

// Touchscreen object
XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);

TFT_eSPI tft;

uint8_t soc = 99;

unsigned long bat_blink = 0;
bool bat_blink_white = true;

void drawBattery()
{
  tft.fillRect(188, 76, 104, 167, TFT_WHITE);
  tft.fillRect(195, 84, 90, 150, TFT_BLACK);
  tft.fillRect(215, 67, 50, 10, TFT_WHITE);
  tft.fillRect(196, 197, 88, 36, TFT_WHITE);
  tft.fillRect(196, 160, 88, 36, TFT_WHITE);
  tft.fillRect(196, 123, 88, 36, TFT_WHITE);
  tft.fillRect(196, 86, 88, 36, TFT_WHITE);
}

void animateBattery()
{
  if (millis() - bat_blink >= 1000)
  {
    uint32_t color = bat_blink_white ? TFT_BLACK : TFT_WHITE;
    bat_blink_white = !bat_blink_white;
    if (soc <= 25)
    {
      tft.fillRect(196, 197, 88, 36, color);
    }
    else if (soc <= 50)
    {
      tft.fillRect(196, 160, 88, 36, color);
    }
    else if (soc <= 75)
    {
      tft.fillRect(196, 123, 88, 36, color);
    }
    else
    {
      tft.fillRect(196, 86, 88, 36, color);
    }
    bat_blink = millis();
  }
}

void drawPage1Info()
{
  tft.setTextSize(1);
  tft.setFreeFont(&FreeSans18pt7b);
  tft.setCursor(33, 89);
  tft.print("54.4v");
  tft.setCursor(14, 191);
  tft.print("-1.06kw");
  tft.setCursor(346, 89);
  tft.print("100%");
  tft.setCursor(327, 191);
  tft.print("-9.5A");
}

void drawBottomNav()
{
  tft.fillTriangle(391, 269, 427, 287, 391, 305, TFT_GREEN);
  tft.fillTriangle(50, 287, 86, 269, 86, 305, TFT_GREEN);
}

void setup()
{
  delay(10000);
  Serial.begin(9600);
  // SPI.begin();  // GPIO14=SCK, GPIO13=MOSI, GPIO12=MISO
  ts.begin();
  ts.setRotation(3); // Adjust rotation to match your TFT
  Serial.println("XPT2046 Touch test");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE); // white text, red background

  drawBattery();
  drawPage1Info();
  drawBottomNav();
}

void loop()
{
  if (ts.touched())
  {

    TS_Point p = ts.getPoint();
    uint16_t x = (p.x * 480) / 4095;
    uint16_t y = (p.y * 320) / 4095;
    Serial.print("X: ");
    Serial.print(x);
    Serial.print(" Y: ");
    Serial.print(y);
    Serial.print(" Z: ");
    Serial.println(p.z);
  }
  animateBattery();
}

// #include <Arduino.h>
// #include <SoftwareSerial.h>
// #include <ModbusMaster.h>

// // MAX485 control pins
// #define DE 0
// #define RE 0

// // RS485 on SoftwareSerial (RO=D2, DI=D3)
// //SoftwareSerial rs485(2, 3); // RX, TX

// // Create ModbusMaster instance
// ModbusMaster node;

// void preTransmission() {
//   digitalWrite(DE, HIGH);
//   digitalWrite(RE, HIGH);
//   delayMicroseconds(300);
// }

// void postTransmission() {
//   digitalWrite(DE, LOW);
//   digitalWrite(RE, LOW);
//   delayMicroseconds(300);
// }

// void setup() {
//   // Control pins
//   pinMode(DE, OUTPUT);
//   pinMode(RE, OUTPUT);
//   digitalWrite(DE, LOW);
//   digitalWrite(RE, LOW);

//   Serial.begin(9600);     // Debugging
//   Serial1.begin(115200);
//   //rs485.begin(9600);      // RS485 baud rate (SoftwareSerial is 8N1 only)

//   // Initialize Modbus communication
//   node.begin(1, Serial);   // Slave ID = 1
//   node.preTransmission(preTransmission);
//   node.postTransmission(postTransmission);
// }

// void loop() {
//   // Send request: function 03, start address 0x1300, read 32 registers
//   uint8_t result = node.readHoldingRegisters(0x1300, 74);

//   if (result == node.ku8MBSuccess) {
//     Serial1.println("Response received:");
//     for (uint8_t i = 0; i < 74; i++) {
//       Serial1.print("Reg ");
//       Serial1.print(i);
//       Serial1.print(": ");
//       Serial1.println(node.getResponseBuffer(i));
//     }
//   } else {
//     Serial1.print("Error code: ");
//     Serial1.println(result);
//   }

//   delay(12000);
// }
