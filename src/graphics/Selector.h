#ifndef SELECTOR_H
#define SELECTOR_H

#include <Arduino.h>   // for String, uint8_t, uint16_t
#include <XPT2046_Touchscreen.h>


class Selector {
public:
  // Constructor
  Selector(String opts[],int8_t current, uint8_t count,
           uint16_t x, uint16_t y, uint16_t tw);

  Selector(uint8_t minV,uint8_t maxV, int8_t val,
           uint16_t x, uint16_t y, uint16_t tw);

  // Draw the selector box, arrows, and current option
  void render();

  // Handle touch input at (px, py)
  void handleTouch(TS_Point p);

  void setCurrentIndex(int8_t current, bool render_value=true ) ;

  void setValue(int8_t value, bool render_value=true );

  // Register a callback to be called when selection changes
  void setOnChange(void (*callback)(uint8_t));

  // Get the currently selected option index
  uint8_t getIndex();

  void setMinValue(uint8_t value);


private:
  // Options data
  String* options;
  int8_t value;        // pointer to array of option strings
  uint8_t optionCount;    // number of options
  int8_t currentIndex;
  uint8_t minValue;
  uint8_t maxValue;
  int8_t currentValue;   // currently selected index


  // Position and dimensions
  uint16_t posX, posY;    // top-left corner of selector box
  uint16_t font_height;
  uint16_t text_width;

  // Arrow positions
  int16_t leftX, leftY;
  int16_t rightX, rightY;

  // Callback function pointer
  void (*onChange)(uint8_t) = nullptr;

  void displayText(String text);
};

#endif // SELECTOR_H
