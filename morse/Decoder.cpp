#include "Decoder.h"

/*  Defines  */

#define COUNTER_MAX     0xFF

/*  Typedefs  */

typedef struct {
    uint8_t code;
    char    letter;
} CODE_T;

/*  Local Functions (macros)  */

#define thresholdLong(f)    ((f) * 5 / 2)
#define thresholdBreak(f)   ((f) * 13 / 2)

/*  Local Constants  */

PROGMEM static const char basicTableEN[] = {
    '\0', '\0', 'E',  'T',  'I',  'A',  'N',  'M',
    'S',  'U',  'R',  'W',  'D',  'K',  'G',  'O',
    'H',  'V',  'F',  '\0', 'L',  '\0', 'P',  'J',
    'B',  'X',  'C',  'Y',  'Z',  'Q',  '\0', '\0',
    '5',  '4',  '\0', '3',  '\0', '\0', '\0', '2',
    '\0', '\0', '+',  '\0', '\0', '\0', '\0', '1',
    '6',  '=',  '/',  '\0', '\0', '\0', '(',  '\0',
    '7',  '\0', '\0', '\0', '8',  '\0', '9',  '0',
};

PROGMEM static const CODE_T extendedTableEN[] = {
    { 64,  '^' }, { 76,  '?' }, { 77,  '_' }, { 82,  '"' }, { 85,  '.' }, { 90,  '@' },
    { 94,  '\''}, { 97,  '-' }, { 109, ')' }, { 115, ',' }, { 120, ':' }, { 0,   '\0'},
};

PROGMEM static const char basicTableJA[] = {
    '\0', '\0', 0x65, 0x76, 0x90, 0x60, 0x6F, 0x6E,
    0x75, 0x77, 0x74, 0x7C, 0x64, 0x6C, 0x68, 0x70,
    0x69, 0x7B, 0x67, 0x79, 0x6D, 0x61, 0x72, 0x6B,
    0x62, 0x7D, 0x63, 0x7E, 0x7F, 0x73, 0x71, 0x80,
    '5',  '4',  '\0', '3',  0x66, 0x88, 0x91, '2',
    0x7A, 0x78, 0x8F, 0x82, 0x8A, 0x92, 0x8D, '1',
    '6',  0x87, 0x8C, 0x86, 0x85, 0x84, 0x6A, 0x81,
    '7',  0x8B, 0x89, 0x83, '8',  0x8E, '9',  '0',
};

PROGMEM static const CODE_T extendedTableJA[] = {
    { 82,  ')' }, { 84,  '\n'}, { 85,  ',' }, { 109, '(' }, { 0,   '\0'},
};

/*---------------------------------------------------------------------------*/

void Decoder::reset(uint8_t frames, uint8_t mode)
{
    unitFrames = frames;
    isJapanese = (mode == DECODE_MODE_JA);
    forceStable();
}

void Decoder::forceStable(void)
{
    isLastSignalOn = false;
    stateCounter = COUNTER_MAX;
    currentCode = CODE_INITIAL;
}

char Decoder::appendSignal(bool isSignalOn)
{
    char ret = '\0';

    if (isSignalOn != isLastSignalOn) {
        isLastSignalOn = isSignalOn;
        stateCounter = 0;
    } else {
        if (stateCounter < COUNTER_MAX) stateCounter++;
    }

    if (isSignalOn) {
        if (stateCounter == 0) {
            if (!(currentCode & 0x100)) currentCode <<= 1;
        } else if (stateCounter == thresholdLong(unitFrames)) {
            currentCode |= 0x01;
        }
    } else {
        if (stateCounter == thresholdLong(unitFrames)) {
            ret = getCandidate();
            if (ret >= '\0' && ret < ' ') stateCounter = COUNTER_MAX;
            currentCode = CODE_INITIAL;
        } else if (stateCounter == thresholdBreak(unitFrames)) {
            ret = ' ';
        }
    }

    isLastSignalOn = isSignalOn;
    return ret;
}

uint16_t Decoder::getCurrentCode(void)
{
    return currentCode;
}

char Decoder::getCandidate(void)
{
    if (currentCode < 64) {
        const char *pTable = (isJapanese) ? basicTableJA : basicTableEN;
        return pgm_read_byte(pTable + currentCode);
    } else if (currentCode < 128) {
        const CODE_T *pTable = (isJapanese) ? extendedTableJA : extendedTableEN;
        uint8_t code;
        while ((code = pgm_read_byte(&pTable->code)) > 0) {
            if (code == currentCode) {
                return pgm_read_byte(&pTable->letter);
            }
            pTable++;
        }
    } else if (!isJapanese && currentCode == 256) {
        return '\b';
    }
    return '\0';
}
