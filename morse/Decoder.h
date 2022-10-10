#pragma once

#include "common.h"

/*  Defines  */

#define CODE_INITIAL    1

/*---------------------------------------------------------------------------*/

class Decoder
{
public:
    void        reset(uint8_t frames, uint8_t mode);
    char        appendSignal(bool isSignalOn);
    void        forceStable(void);
    uint16_t    getCurrentCode(void);
    char        getCandidate(void);

private:
    uint16_t    currentCode;
    uint8_t     unitFrames, stateCounter;
    bool        isLastSignalOn, isJapanese, isParenthesisOpen;
};
