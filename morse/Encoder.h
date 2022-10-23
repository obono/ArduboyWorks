#pragma once

#include "common.h"

class Encoder
{
public:
    void        reset(uint8_t frames, uint8_t mode);
    void        setLetter(char letter);
    uint16_t    getMorseCode(char letter);
    bool        forwardFrame(void);
    bool        isEncoding(void);

private:
    uint16_t    currentCode, codeMask;
    uint8_t     unitFrames, stateCounter;
    bool        isJapanese;
};
