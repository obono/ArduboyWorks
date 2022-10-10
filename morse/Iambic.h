#pragma once

#include "common.h"

/*---------------------------------------------------------------------------*/

class Iambic
{

enum State : uint8_t
{
    WAITING = 0,
    RESERVED,
    SENDING,
};

public:
    void    reset(uint8_t frames);
    bool    isSignalOn(bool isShortOn, bool isLongOn);

private:
    Iambic::State state;
    uint8_t unitFrames, stateCounter;
    bool    isSendLong;
};
