#pragma once

#include <Arduboy2.h>
#if ARDUBOY_LIB_VER < 50201
#error Unexpected version of Arduboy Library
#endif // It may work even if you use other version. So comment out the above line.

class MyArduboy2 : public Arduboy2
{
public:
    void    beginNoLogo(void);
    bool    nextFrame(void);
    bool    buttonDown(uint8_t buttons);
    bool    buttonPressed(uint8_t buttons);
    bool    buttonUp(uint8_t buttons);
    void    setTextColors(uint8_t color, uint8_t bg);
    virtual size_t write(uint8_t);
    template <typename T> size_t printEx(int16_t x, int16_t y, T p) { setCursor(x, y); return print(p); }
    void    drawRect(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t color);
    void    drawFastVLine(int16_t x, int16_t y, int8_t h, uint8_t color);
    void    drawFastHLine(int16_t x, int16_t y, uint8_t w, uint8_t color);
    void    fillRect(int16_t x, int16_t y, uint8_t w, int8_t h, uint8_t color);

    void    initAudio(uint8_t chans);
    void    closeAudio(void);
    bool    isAudioEnabled(void);
    void    setAudioEnabled(bool on);
    void    toggleAudioEnabled(void);
    void    saveAudioOnOff(void);
    void    playTone(uint16_t frequency, uint16_t duration, uint8_t priority = 0xFF, uint8_t dutyCycle = 2);
    void    playScore(const byte *score, uint8_t priority = 0, int8_t pitch = 0);
    void    stopScore(void);

private:
    void    drawChar(int16_t x, int16_t y, unsigned char c, uint8_t color, uint8_t bg, uint8_t size);
    void    fillBeltBlack(uint8_t *p, uint8_t d, uint8_t w);
    void    fillBeltWhite(uint8_t *p, uint8_t d, uint8_t w);
    uint8_t lastButtonState;
    uint8_t currentButtonState;
};
