#ifndef MYARDUBOY_H
#define MYARDUBOY_H

#include <Arduboy.h>

class MyArduboy : public Arduboy
{
public:
    bool nextFrame();
    bool buttonDown(uint8_t buttons);
    bool buttonPressed(uint8_t buttons);
    bool buttonUp(uint8_t buttons);
    void setTextColor(uint8_t color);
    void setTextColor(uint8_t color, uint8_t bg);
    virtual size_t write(uint8_t);
    void drawRect2(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t color);
    void drawFastVLine2(int16_t x, int16_t y, int8_t h, uint8_t color);
    void drawFastHLine2(int16_t x, int16_t y, uint8_t w, uint8_t color);
    void fillRect2(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t color);

private:
    void    myDrawChar(int16_t x, int16_t y, unsigned char c, uint8_t color, uint8_t bg, uint8_t size);
    void    fillBeltBlack(unsigned char *p, unsigned char d, uint8_t w);
    void    fillBeltWhite(unsigned char *p, unsigned char d, uint8_t w);
    uint8_t textcolor = WHITE;
    uint8_t textbg = BLACK;
    uint8_t lastButtonState;
    uint8_t currentButtonState;
};

#endif // MYARDUBOY_H
