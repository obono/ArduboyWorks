#include "Encoder.h"

/*  Defines  */

#define CODE_INITIAL    1

/*  Local Functions (macros)  */

#define getSignalFrames(frames, isLong) ((frames) * ((isLong) ? 4 : 2))

/*  Local Constants  */

PROGMEM static const uint8_t encodeTable[] = {
    0,   0,   82,  0,   0,   0,   0,   94,  54,  109, 0,   42,  115, 97,  85,  50,
    63,  47,  39,  35,  33,  32,  48,  56,  60,  62,  120, 0,   0,   49,  0,   76,
    90,  5,   24,  26,  12,  2,   18,  14,  16,  4,   23,  13,  20,  7,   6,   15,
    22,  29,  10,  8,   3,   9,   17,  11,  25,  27,  28,  0,   0,   0,   64,  77,
    5,   21,  24,  26,  12,  2,   36,  18,  14,  16,  54,  23,  13,  20,  7,   6,
    15,  30,  22,  29,  10,  8,   3,   9,   41,  19,  40,  17,  11,  25,  27,  28,
    31,  55,  43,  59,  53,  52,  51,  49,  37,  58,  44,  57,  50,  46,  61,  42,
    4,   38,  45,  85,
};

/*---------------------------------------------------------------------------*/

void Encoder::reset(uint8_t frames, uint8_t mode)
{
    unitFrames = frames;
    isJapanese = (mode == DECODE_MODE_JA);
    currentCode = 0;
}

void Encoder::setLetter(char letter)
{
    currentCode = getMorseCode(letter);
    if (currentCode > CODE_INITIAL) {
        for (codeMask = 0x100; !(currentCode & codeMask); codeMask >>= 1) { ; }
        codeMask >>= 1;
        stateCounter = getSignalFrames(unitFrames, currentCode & codeMask);
    }
}

uint16_t Encoder::getMorseCode(char letter)
{
    uint16_t ret = 0;
    uint8_t idx = letter - 0x20;

    if (isJapanese) {
        if (letter >= '0' && letter <= '9' || letter >= 'A' && letter <= 'Z' ||
                idx >= 0x40 && idx <= 0x73) {
            ret = pgm_read_byte(&encodeTable[idx]);
        } else if (letter == '(') {
            ret = 109;
        } else if (letter == ')') {
            ret = 82;
        }
    } else {
        if (letter >= ' ' && letter <= '_') {
            ret = pgm_read_byte(&encodeTable[idx]);
        }
    }

    return ret;
}

bool Encoder::forwardFrame(void)
{
    if (currentCode <= CODE_INITIAL) return false;

    bool ret = (stateCounter > unitFrames);
    if (--stateCounter == 0) {
        codeMask >>= 1;
        if (codeMask == 0) {
            currentCode = 0;
        } else {
            stateCounter = getSignalFrames(unitFrames, currentCode & codeMask);
        }
    }
    return ret;
}

bool Encoder::isEncoding(void)
{
    return (currentCode > CODE_INITIAL);
}
