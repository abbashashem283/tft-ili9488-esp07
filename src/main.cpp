#include <Arduino.h>

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <ModbusMaster.h>
#include "graphics/Selector.h"

// Pins
#define CS_PIN 16   // Chip select for XPT2046
#define TIRQ_PIN -1 // Optional, can be -1 if unused

#define DE 0
#define RE 0

#define MAX_PAGE_NB 3
// Touchscreen object
XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);

TFT_eSPI tft;
uint16_t sw, sh;

ModbusMaster node;
bool modbus_busy = false;
uint8_t page = 1;
uint8_t soc = 0;
uint8_t font_height = 0;
float voltage = 0.0, current = 0.0, watts = 0.0;

uint16_t charge_color = TFT_WHITE;

String baud_rates[] = {"1200","2400","4800","9600", "14400", "19200", "38400", "56000","57600","115200"};
uint8_t selected_baud = 3;

unsigned long bat_blink = 0;
unsigned long touch = 0;
bool bat_blink_white = true;

unsigned long page1Info = 0;

bool is_charging = false, is_idle = false, is_discharging = false, is_error = false;

uint8_t nb_of_cells = 0;


Selector baudSelector(baud_rates, 10, 240, 10, 200);
Selector slaveSelector(1, 50, 240, 70, 200);
Selector cutOffSelector(20, 100, 240, 130, 200);
Selector turnONSelector(25, 100, 240, 190, 200);

void preTransmission()
{
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  //delayMicroseconds(300);
}

void postTransmission()
{
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
  //delayMicroseconds(300);
}

void printString(String text, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16 fc = TFT_WHITE, const GFXfont *font = &FreeSans18pt7b)
{
  TFT_eSprite sprite = TFT_eSprite(&tft);
  sprite.createSprite(w, h);

  // Explicit font and colors
  sprite.setFreeFont(font); // match whatever you used on tft
  sprite.setTextColor(fc , TFT_BLACK);

  // Fill background
  //sprite.fillSprite(TFT_BLUE);

  // Draw text with baseline offset
  sprite.drawString(text, 0, 0);

  // Push to screen
  sprite.pushSprite(x, y);
  sprite.deleteSprite();
}

void batteryChargeModeChanged()
{
  if (is_idle)
    charge_color = TFT_WHITE;
  else if (is_charging)
    charge_color = TFT_GREEN;
  else if (is_discharging)
    charge_color = TFT_YELLOW;
}

void drawBatterySOC()
{

  if (soc > 0)
    tft.fillRect(196, 197, 88, 36, TFT_WHITE);
  if (soc >= 50)
    tft.fillRect(196, 160, 88, 36, TFT_WHITE);
  if (soc > 50)
    tft.fillRect(196, 123, 88, 36, TFT_WHITE);
  if (soc >= 75)
    tft.fillRect(196, 86, 88, 36, TFT_WHITE);
  if (is_discharging)
  {
    if (soc < 80)
      tft.fillRect(196, 86, 88, 36, TFT_BLACK);
    if (soc <= 50)
      tft.fillRect(196, 123, 88, 36, TFT_BLACK);
    if (soc <= 25)
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



void clearPage(uint8_t p)
{
  switch (p)
  {
  case 1:
    tft.fillRect(188, 67, 104, 176, TFT_BLACK);
    tft.fillRect(33, 89, 135, font_height, TFT_BLACK);
    tft.fillRect(33, 191, 135, font_height, TFT_BLACK);
    tft.fillRect(357, 89, 110, font_height, TFT_BLACK);
    tft.fillRect(357, 191, 110, font_height, TFT_BLACK);
    break;

  case 2:
  {
    uint16_t x = 40, y = 10;
    for (uint8_t i = 0; i < 4; ++i)
    {
      tft.fillRect(x, y, 410, 25, TFT_BLACK);
      x = 40;
      y += 70;
    }
    break;
  }
  default:
    break;
  }
}

void errorStatChanged(uint8_t result) {
  if(is_error)
    printString("COM ERROR: " + String(result), 140, 284, 200, font_height, TFT_RED, &FreeSans12pt7b);
  else
    tft.fillRect(140,284,200,font_height,TFT_BLACK);        
}

void modbusInfo(){
  uint8_t result = node.readHoldingRegisters(0x1300, 74);
  bool old_error = is_error ;
  is_error = result != node.ku8MBSuccess ;
  if(is_error != old_error) errorStatChanged(result);
}

void renderPage(uint8_t p)
{
  p = p > MAX_PAGE_NB ? 1 : p;
  p = p < 1 ? MAX_PAGE_NB : p;

  if (p != page)
  {
    clearPage(page);
  }

  if(is_error) return ;
    
    uint16_t raw_voltage = node.getResponseBuffer(6);
    switch (p)
    {
    case 1:
    {
      if (p != page)
      {
        drawBattery();
        drawBatterySOC();
        drawPage1Info();
      }
      float new_voltage = raw_voltage / 100.0f;
      if(new_voltage != voltage){
        voltage = new_voltage;
        printString(String(voltage) + "V", 33, 89, 135, font_height);
      }
      uint8_t newSOC = node.getResponseBuffer(11);
      if (soc != newSOC)
      {
        soc = newSOC;
        printString(String(soc) + "%", 357, 89, 110, font_height);
        drawBatterySOC();
      }
      uint16_t raw_current = node.getResponseBuffer(7);
      float new_current = 0.0;
      if (raw_current == 0)
      {
        if (!is_idle)
        {
          is_idle = true;
          batteryChargeModeChanged();
        }
        is_charging = false;
        is_discharging = false;
      }
      else if (raw_current >= 30000)
      {
        if (is_charging == false)
        {
          is_charging = true;
          batteryChargeModeChanged();
        }

        is_idle = false;
        is_discharging = false;
        new_current = (65535 - raw_current) / 10.0f;
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
        new_current = raw_current / 10.0f;
      }
      if(current != new_current){
        current = new_current;
        watts = (current * idealVoltage(voltage)) / 1000.0f;
        printString(String(watts) + "kW", 33, 191, 135, font_height, charge_color);
        printString(String(current, 1) + "A", 357, 191, 110, font_height, charge_color);
      }
      //drawPage1Info();
      break;
    }
    case 2:
    {
      uint16_t x = 40, y = 10;
      uint8_t r = 42;
      uint16_t accumulated_volts = 0.0;
      static std::vector<uint16_t> raw_cell_values;
      // Serial1.print("page = ");
      // Serial1.print(page);
      // Serial1.println(" vs " + String(raw_cell_values.size()));
      if (page != 2)
        raw_cell_values.clear();
      if (raw_cell_values.size() >= 100)
        raw_cell_values.clear();
      for (uint8_t i = 0; i < 100; ++i)
      {
        uint8_t vs = raw_cell_values.size();
        uint16_t raw_cell_volt = node.getResponseBuffer(r);
        float cell_volt = raw_cell_volt / 1000.0f;
        accumulated_volts += raw_cell_volt;
        if (i >= vs || std::abs(raw_cell_values.at(i) - raw_cell_volt) >= 20)
        {
          //Serial1.println(cell_volt);
          printString(String(cell_volt, 2), x, y, 55, font_height, TFT_WHITE, &FreeSans12pt7b);
          if (i < vs)
            raw_cell_values[i] = raw_cell_volt;
          else
            raw_cell_values.push_back(raw_cell_volt);
        }
        x += 110;
        if ((i + 1) % 4 == 0)
        {
          x = 40;
          y += 70;
        }
        // Serial1.print("av ");
        // Serial1.print(accumulated_volts);
        // Serial1.print(" v ");
        // Serial1.print(raw_voltage);
        // Serial1.println("=..=..=..=..=..=..=..=..=");
        if (raw_voltage * 10 - accumulated_volts <= 50)
          break;
        ++r;
      }
      break;
    }
    case 3: {
      if(page == 3) return;
      printString("Baud",10,10,tft.textWidth("Baud"),font_height,TFT_WHITE);
      baudSelector.render();
      printString("SID",10,70,tft.textWidth("SID"),font_height,TFT_WHITE);
      slaveSelector.render();
      printString("Cut Off",10,130,tft.textWidth("Cut Off"),font_height,TFT_WHITE);
      cutOffSelector.render();
      printString("Turn ON",10,190,tft.textWidth("Turn ON"),font_height,TFT_WHITE);
      turnONSelector.render();
      break;
    }
  }
  
 

  page = p;
}

// void animateBattery()
// {
//   if (is_error || !is_charging || page != 1)
//     return;
//   if (millis() - bat_blink >= 1000)
//   {
//     uint32_t color = bat_blink_white ? TFT_BLACK : TFT_WHITE;
//     bat_blink_white = !bat_blink_white;
//     // drawBatterySOC();
//     if (soc <= 25)
//     {
//       tft.fillRect(196, 197, 88, 36, color);
//     }
//     else if (soc <= 50)
//     {
//       tft.fillRect(196, 160, 88, 36, color);
//     }
//     else if (soc <= 75)
//     {
//       tft.fillRect(196, 123, 88, 36, color);
//     }
//     else
//     {
//       tft.fillRect(196, 86, 88, 36, color);
//     }
//     bat_blink = millis();
//   }
// }

void drawBottomNav()
{
  tft.fillTriangle(391, 269, 427, 287, 391, 305, TFT_GREEN);
  tft.fillTriangle(50, 287, 86, 269, 86, 305, TFT_GREEN);
}

bool touchIn(TS_Point p, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint16_t px = (p.x * sw) / 4095;
  uint16_t py = (p.y * sh) / 4095;
  Serial.println("touch in "+ String(px) + " "+ String(py));
  return (px >= x && px <= x + w && py >= y && py <= y + h);
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
  delay(3000);
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

  sw = tft.width();
  sh = tft.height();

  

  // drawBattery();
  // drawPage1Info();
   drawBottomNav();

  
  //baudSelector = Selector(baud_rates, 10, 100, 100, 200);
  renderPage(3);
  //tft.drawRect(270,140,60,60, TFT_CYAN);
  //tft.drawRect(70,240,60,60, TFT_CYAN);
}



void loop()
{

  if (ts.touched() && millis() - touch >= 250)
  {
    TS_Point p = ts.getPoint();
    if (touchIn(p, 350, 230, 46, 46))
    {
      // Serial.println("x y touch");
      renderPage(page + 1);
    }
    if (touchIn(p, 55, 250, 46, 46))
    {

      renderPage(page - 1);
    }
    if(page == 3){
      baudSelector.handleTouch(p);
      slaveSelector.handleTouch(p);
    }
    touch = millis();
  }
    // getBatteryInfo();
    if(millis() - page1Info >= 1000){
      //modbusInfo();
      //renderPage(page);
      page1Info= millis();
    }
    
  

  // if(millis() - test >= 5000){
  //     renderPage(p1 ? 1 : 2);
  //     p1 = !p1 ;
  //     test = millis();
  // }

  // if (millis() - page1Info >= 500)
  // {
  //   // getBatteryInfo();
  //   renderPage(page);
  //   page1Info = millis();
  // }

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
