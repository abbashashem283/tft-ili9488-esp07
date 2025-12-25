#include <Arduino.h>

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <ModbusMaster.h>

// Pins
#define CS_PIN 16   // Chip select for XPT2046
#define TIRQ_PIN -1 // Optional, can be -1 if unused

#define DE 0
#define RE 0
// Touchscreen object
XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);

TFT_eSPI tft;

ModbusMaster node;

uint8_t soc = 0;
uint8_t font_height = 0;
float voltage = 0.0, current = 0.0, watts = 0.0;

uint16_t charge_color = TFT_WHITE;

unsigned long bat_blink = 0;
bool bat_blink_white = true;

unsigned long page1Info = 0;

bool is_charging = false, is_idle = true, is_discharging = false, is_error = false;

void preTransmission()
{
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delayMicroseconds(300);
}

void postTransmission()
{
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
  delayMicroseconds(300);
}

void printString(String text, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16 fc = TFT_WHITE)
{
  TFT_eSprite sprite = TFT_eSprite(&tft);
  sprite.createSprite(w, h);

  // Explicit font and colors
  sprite.setFreeFont(&FreeSans18pt7b); // match whatever you used on tft
  sprite.setTextColor(fc, TFT_BLACK);

  // Fill background
  sprite.fillSprite(TFT_BLACK);

  // Draw text with baseline offset
  sprite.drawString(text, 0, 0);

  // Push to screen
  sprite.pushSprite(x, y);
  sprite.deleteSprite();
}

void batteryChargeModeChanged()
{
  if (is_charging)
    charge_color = TFT_GREEN;
  else if (is_discharging)
    charge_color = TFT_YELLOW;
}

void drawBatterySOC()
{
  if (is_error)
    return;
  if (is_charging || is_idle)
  {
    if (soc > 0)
      tft.fillRect(196, 197, 88, 36, TFT_WHITE);
    if (soc >= 50)
      tft.fillRect(196, 160, 88, 36, TFT_WHITE);
    if (soc >= 75)
      tft.fillRect(196, 123, 88, 36, TFT_WHITE);
    if (soc >= 80)
      tft.fillRect(196, 86, 88, 36, TFT_WHITE);
  }
  else if (is_discharging)
  {
    if (soc < 80)
      tft.fillRect(196, 86, 88, 36, TFT_BLACK);
    if (soc < 75)
      tft.fillRect(196, 123, 88, 36, TFT_BLACK);
    if (soc < 50)
      tft.fillRect(196, 160, 88, 36, TFT_BLACK);
    if (soc <= 0)
      tft.fillRect(196, 197, 88, 36, TFT_BLACK);
  }
}

void drawBattery()
{
  tft.fillRect(188, 76, 104, 167, TFT_WHITE);
  tft.fillRect(195, 84, 90, 150, TFT_BLACK);
  tft.fillRect(215, 67, 50, 10, TFT_WHITE);
  drawBatterySOC();
}

void drawPage1Info()
{
  printString(String(voltage) + "V", 33, 89, 135, font_height);
  printString(String(watts) + "kW", 33, 191, 135, font_height, charge_color);
  printString(String(soc) + "%", 357, 89, 110, font_height);
  printString(String(current, 1) + "A", 357, 191, 110, font_height, charge_color);
}

uint8_t idealVoltage(float voltage)
{
  uint8_t nominal = 0;
  if (voltage >= 12)
    nominal = 12;
  if (voltage >= 24)
    nominal = 24;
  if (voltage >= 48)
    nominal = 48;
  return nominal;
}

void getBatteryInfo()
{
  uint8_t result = node.readHoldingRegisters(0x1300, 74);

  if (result == node.ku8MBSuccess)
  {
    uint8_t newSOC = node.getResponseBuffer(11);
    if (soc != newSOC)
    {
      soc = newSOC;
      drawBatterySOC();
    }
    voltage = node.getResponseBuffer(6) / 100.0f;
    uint16_t raw_current = node.getResponseBuffer(7);
    if (raw_current >= 30000)
    {
      if (is_charging == false)
      {
        is_charging = true;
        batteryChargeModeChanged();
      }

      is_idle = false;
      is_discharging = false;
      current = (65535 - raw_current) / 10.0f;
    }
    else
    {
      if (is_discharging == false)
      {
        is_discharging = true;
        batteryChargeModeChanged();
      }
      is_charging = false;
      is_idle = false;
      current = raw_current / 10.0f;
    }
    watts = (current * idealVoltage(voltage)) / 1000.0f;
  }
  else
  {
    is_error = true;
    tft.setCursor(10, 30);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("COM Error: ");
    tft.print(result);
    tft.setFreeFont(&FreeSans18pt7b);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }
}

void animateBattery()
{
  if (is_error || !is_charging)
    return;
  if (millis() - bat_blink >= 1000)
  {
    uint32_t color = bat_blink_white ? TFT_BLACK : TFT_WHITE;
    bat_blink_white = !bat_blink_white;
    drawBatterySOC();
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

void drawBottomNav()
{
  tft.fillTriangle(391, 269, 427, 287, 391, 305, TFT_GREEN);
  tft.fillTriangle(50, 287, 86, 269, 86, 305, TFT_GREEN);
}

void setup()
{
  pinMode(DE, OUTPUT);
  pinMode(RE, OUTPUT);
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  Serial.begin(9600); // Debugging
  Serial1.begin(115200);
  // rs485.begin(9600);      // RS485 baud rate (SoftwareSerial is 8N1 only)

  // Initialize Modbus communication
  node.begin(1, Serial); // Slave ID = 1
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  delay(10000);
  // SPI.begin();  // GPIO14=SCK, GPIO13=MOSI, GPIO12=MISO
  ts.begin();
  ts.setRotation(3); // Adjust rotation to match your TFT
  Serial1.println("XPT2046 Touch test");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // white text, red background
  tft.setTextSize(1);
  tft.setFreeFont(&FreeSans18pt7b);
  font_height = tft.fontHeight();

  drawBattery();
  drawPage1Info();
  drawBottomNav();
  // TFT_eSprite sprite = TFT_eSprite(&tft);
  // sprite.createSprite(200, 100);
  // sprite.fillSprite(TFT_BLUE);
  // sprite.pushSprite(50, 50);
  // sprite.deleteSprite();
  // printString("M",50,50,28,font_height);
}

void loop()
{

  if (ts.touched())
  {

    TS_Point p = ts.getPoint();
    uint16_t x = (p.x * 480) / 4095;
    uint16_t y = (p.y * 320) / 4095;
    Serial1.print("X: ");
    Serial1.print(x);
    Serial1.print(" Y: ");
    Serial1.print(y);
    Serial1.print(" Z: ");
    Serial1.println(p.z);
  }
  animateBattery();

  if (millis() - page1Info >= 500)
  {
    getBatteryInfo();
    drawPage1Info();
    page1Info = millis();
  }
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
