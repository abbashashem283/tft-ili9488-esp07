#include "Button.h"


extern TFT_eSPI tft;
extern void printString(String text, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t fc, const GFXfont *font, uint16_t bc);
extern bool touchIn(TS_Point p, uint16_t x, uint16_t y, uint16_t w, uint16_t h);



Button::Button(String text, uint16_t background,uint16_t w, uint16_t h, uint16_t foreground,  bool is_rounded):
label(text),
fc(foreground),
bc(background),
width(w),
height(h),
rounded(is_rounded)
{}

void Button::render(uint16_t x, uint16_t y){
    tft.setFreeFont(&FreeSans12pt7b);
    this->x = x;
    this->y = y;
    uint8_t radius = rounded ? 12 : 0;
    uint16_t font_width = tft.textWidth(label);
    uint16_t font_height = tft.fontHeight() - 10;
    uint16_t remaining_space_x = width - font_width;
    uint16_t remaining_space_y = height - font_height;
    float padding_x = remaining_space_x / 2;
    float padding_y = remaining_space_y / 2;
    tft.fillRoundRect(x,y,width,height,radius, bc);
    printString(label, x+padding_x,y+padding_y,font_width,font_height,fc, &FreeSans12pt7b, bc);
    tft.setFreeFont(&FreeSans18pt7b);
}

void Button::setOnClick(void (*callback)()){
    onClick = callback ;
}

void Button::clickListener(TS_Point p) {
    if(onClick && touchIn(p, x, y, width, height))
        onClick();
}

