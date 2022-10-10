#include "Iambic.h"

void Iambic::reset(uint8_t frames)
{
    unitFrames = frames;
    state = WAITING;
    stateCounter = 0;
    isSendLong = false;
}

bool Iambic::isSignalOn(bool isShortOn, bool isLongOn)
{
    if (stateCounter > 0) stateCounter--;
    if (state == SENDING && stateCounter == 0) {
        state = WAITING;
        stateCounter = unitFrames;
    }
    if (state == WAITING) {
        if (isShortOn && (!isLongOn || isSendLong)) {
            state = RESERVED;
            isSendLong = false;
        } else if (isLongOn && (!isShortOn || !isSendLong)) {
            state = RESERVED;
            isSendLong = true;
        }
    }
    if (state == RESERVED && stateCounter == 0) {
        state = SENDING;
        stateCounter = (isSendLong) ? unitFrames * 3 : unitFrames;
    }
    return (state == SENDING);
}
