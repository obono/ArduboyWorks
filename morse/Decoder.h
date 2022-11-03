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
    void        forceStable(bool isResetParenthesis = false);
    uint16_t    getCurrentCode(void);
    char        getCandidate(void);

private:
    uint8_t     unitFrames, stateCounter;
    uint16_t    currentCode;
    bool        isLastSignalOn, isJapanese, isParenthesisOpen;
};
