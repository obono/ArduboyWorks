#ifndef MYARDUBOY_H
#define MYARDUBOY_H

//#define USE_ARDUBOY2_LIB

#ifdef USE_ARDUBOY2_LIB
#include <Arduboy2.h>
#include <ArduboyPlaytune.h>
#define ARDUBOY_LIB_CLASS Arduboy2
#define ARDUBOY_LIB_VER_TGT 40100
typedef uint8_t buffer_t;
#else
#include <Arduboy.h>
#define ARDUBOY_LIB_CLASS Arduboy
#define ARDUBOY_LIB_VER_TGT 10101
typedef unsigned char buffer_t;
#endif

class MyArduboy : public ARDUBOY_LIB_CLASS
{
public:
#ifdef USE_ARDUBOY2_LIB
    MyArduboy(void);
    void    beginNoLogo(void);
#endif
    bool    nextFrame(void);
    bool    buttonDown(uint8_t buttons);
    bool    buttonPressed(uint8_t buttons);
    bool    buttonUp(uint8_t buttons);
    void    setTextColor(uint8_t color);
    void    setTextColor(uint8_t color, uint8_t bg);
    size_t  printEx(int16_t x, int16_t y, const char *p);
    size_t  printEx(int16_t x, int16_t y, const __FlashStringHelper *p);
    virtual size_t write(uint8_t);
    void    drawRect2(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t color);
    void    drawFastVLine2(int16_t x, int16_t y, int8_t h, uint8_t color);
    void    drawFastHLine2(int16_t x, int16_t y, uint8_t w, uint8_t color);
    void    fillRect2(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t color);
    void    playScore2(const byte *score, uint8_t priority);
    void    tone2(unsigned int frequency, unsigned long duration);

private:
    void    myDrawChar(int16_t x, int16_t y, unsigned char c, uint8_t color, uint8_t bg, uint8_t size);
    void    fillBeltBlack(buffer_t *p, uint8_t d, uint8_t w);
    void    fillBeltWhite(buffer_t *p, uint8_t d, uint8_t w);
    uint8_t textcolor = WHITE;
    uint8_t textbg = BLACK;
    uint8_t lastButtonState;
    uint8_t currentButtonState;
    uint8_t playScorePriority;
#ifdef USE_ARDUBOY2_LIB
    ArduboyPlaytune *pTunes;
#endif
};

#endif // MYARDUBOY_H
