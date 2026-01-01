#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

class Button {
    public:
        Button(String label,uint16_t bc, uint16_t w, uint16_t h,uint16_t fc = TFT_BLACK, bool rounded= false);
        void setOnClick(void (*callback)());
        void clickListener(TS_Point p);
        void render(uint16_t x, uint16_t y);
    
    private:
        String label;
        uint16_t fc;  
        uint16_t bc;
        uint16_t width, height;
        uint16_t x,y;
        bool rounded; 
        void (*onClick)() = nullptr; 
};