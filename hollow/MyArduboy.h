#ifndef MYARDUBOY_H
#define MYARDUBOY_H

#include <Arduboy.h>

class MyArduboy : public Arduboy
{
public:
    virtual size_t write(uint8_t);
    void setTextColor(uint8_t color);
    void setTextColor(uint8_t color, uint8_t bg);

private:
    void myDrawChar(int16_t x, int16_t y, unsigned char c, uint8_t color, uint8_t bg, uint8_t size);
    uint8_t textcolor = WHITE;
    uint8_t textbg = BLACK;
};

#endif // MYARDUBOY_H
