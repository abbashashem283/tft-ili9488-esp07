#include "Selector.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern void printString(String text, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t fc, const GFXfont *font, uint16_t bc);
extern bool touchIn(TS_Point p, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

void arrow_button(uint16_t x,
                  uint16_t y,
                  uint16_t bgColor,
                  uint16_t fgColor,
                  uint16_t size,
                  bool is_left = false)
{
    uint16_t r = size;

    // Draw the circle (background)
    tft.fillCircle(x, y, r, bgColor);

    // Triangle dimensions
    uint16_t triWidth = r * 1.2;
    uint16_t triHeight = r * 1.2;

    // Base (left) of arrow
    int16_t x0 = x - triWidth / 2;
    int16_t y0 = y - triHeight / 2;

    // Base (left, bottom)
    int16_t x1 = x0;
    int16_t y1 = y + triHeight / 2;

    // Tip (right)
    int16_t x2 = x + triWidth / 2;
    int16_t y2 = y;

    // If arrow should point left — flip horizontally
    if (is_left)
    {
        int16_t flipped_x0 = x + triWidth / 2;
        int16_t flipped_x1 = flipped_x0;
        int16_t flipped_x2 = x - triWidth / 2;

        x0 = flipped_x0;
        x1 = flipped_x1;
        x2 = flipped_x2;
    }

    tft.fillTriangle(x0, y0, x1, y1, x2, y2, fgColor);
}

Selector::Selector(String opts[],int8_t current, uint8_t count, uint16_t x, uint16_t y, uint16_t tw) : 
options(opts), 
optionCount(count), 
currentIndex(current), 
posX(x), 
posY(y), 
font_height(30), 
text_width(tw),
onChange(nullptr) {
    
}

Selector::Selector(uint8_t minV, uint8_t maxV, int8_t val, uint16_t x, uint16_t y, uint16_t tw) : 
minValue(minV),
maxValue(maxV),
optionCount(0),
currentValue(val),
currentIndex(-1),
posX(x),
posY(y),
font_height(30),
text_width(tw),
onChange(nullptr) {
                                                                                           
}

void Selector::displayText(String text)
{
    uint8_t input_width = tft.textWidth(text);
    uint8_t remaining_space = text_width - input_width;
    tft.fillRect(posX + 20, posY, text_width - 40, font_height, TFT_BLACK);

    printString(text, posX + remaining_space / 2, posY, input_width, font_height, TFT_WHITE, &FreeSans18pt7b, TFT_BLACK);
}

void Selector::setMinValue(uint8_t value){
    minValue = value;
    if(currentValue < value)
        setValue(value);
}

void Selector::setValue(int8_t val, bool render_value )
{
    currentValue = val;   
    if(val < minValue)
        currentValue = maxValue;
    else if(val > maxValue)
        currentValue = minValue;    
    //Serial1.println("value is "+String(currentValue));
    if(render_value)
        displayText(String(currentValue));
    if(onChange)    
     onChange(currentValue);    
}

uint8_t Selector::getValue() {
    return currentValue;
}

void Selector::setCurrentIndex(int8_t index, bool render_value)
{
    //Serial1.println("index: " + String(index) + " currentIndex: " + String(currentIndex));
    currentIndex = index;
    if (currentIndex < 0)
        currentIndex = optionCount - 1;
    else if (currentIndex >= optionCount)
        currentIndex = 0;
    String selected_text = options[currentIndex];
    if(render_value)
        displayText(selected_text);
    if(onChange)    
     onChange(currentIndex);    
}

void Selector::render()
{
    uint16_t lx = posX;
    uint16_t ly = posY + 20;
    // static bool is_rendered = false;
    // if(!is_rendered)
    arrow_button(lx, ly, TFT_GREEN, TFT_BLACK, 20, true);
    String option;
    if (currentIndex == -1)
        option = String(currentValue);
    else
        option = options[currentIndex];
    ////Serial1.println("val is " + String(value) + " and currentIndex is " + String(currentIndex));
    uint8_t input_width = tft.textWidth(option);
    uint8_t remaining_space = text_width - input_width;
    printString(option, posX + remaining_space / 2, posY, input_width, font_height, TFT_WHITE, &FreeSans18pt7b, TFT_BLACK);
    uint16_t rx = posX + text_width;
    uint16_t ry = posY + 20;
    // if(!is_rendered)
    arrow_button(rx, ry, TFT_GREEN, TFT_BLACK, 20);
}

void Selector::setOnChange(void (*callback)(uint8_t)){
    onChange =callback ;
}

void Selector::handleTouch(TS_Point p)
{
    uint16_t lx = posX;
    uint16_t ly = posY + 20;
    uint16_t rx = posX + text_width;
    uint16_t ry = posY + 20;
    //Serial1.println("current index "+ String(currentIndex));
    if (touchIn(p, lx - 30, ly - 30, 60, 60))
    {
        if(currentIndex == -1)
            this->setValue(currentValue - 1);
        else
            this->setCurrentIndex(currentIndex - 1);
    }
    if (touchIn(p, rx - 30, ry - 30, 60, 60)){
        if(currentIndex == -1)
            this->setValue(currentValue + 1);
        else
            this->setCurrentIndex(currentIndex + 1);
    }
}
