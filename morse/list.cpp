#include "common.h"
#include "Encoder.h"

/*  Defines  */

#define CHARS_ALPHABET  26
#define CHARS_JAPANESE  52
#define CHARS_NUMBER    10

#define CHARS_IN_LIST_EN (CHARS_ALPHABET + CHARS_NUMBER + sizeof(extendedListEN))
#define CHARS_IN_LIST_JA (CHARS_JAPANESE + CHARS_NUMBER + sizeof(extendedListJA))

#define CHARS_IN_COLUMN 8
#define COLUMNS_IN_PAGE 3

/*  Local Functions  */

static char getLetterInList(uint8_t idx, bool isJapanese);

/*  Local Constants  */

PROGMEM static const char extendedListEN[] = {
    '.', ',', ':', '?', '\'', '-', '(', ')', '/', '=', '+', '"', '@', '^', '_'
};

PROGMEM static const char extendedListJA[] = {
    '(', ')'
};

/*  Local Variables  */

static Encoder  encoder;
static uint8_t  currentIdx, baseIdx;
static bool     isJapanese, isLastSignalOn;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initList(void)
{
    encoder.reset(record.unitFrames + 2, record.decodeMode);
    currentIdx = 0;
    baseIdx = 0;
    isJapanese = (record.decodeMode == DECODE_MODE_JA);
    isLastSignalOn = false;
    isInvalid = true;
}

MODE_T updateList(void)
{
    MODE_T nextMode = MODE_LIST;
    handleDPad();
    if (arduboy.buttonDown(A_BUTTON)) {
        nextMode = MODE_CONSOLE;
        indicateSignalOff();
        playSoundClick();
    } else if (arduboy.buttonDown(B_BUTTON)) {
        encoder.setLetter(getLetterInList(currentIdx, isJapanese));
    } else {
        uint8_t maxIdx = (isJapanese) ? CHARS_IN_LIST_JA : CHARS_IN_LIST_EN;
        if (padX < 0 && currentIdx < CHARS_IN_COLUMN ||
                padX > 0 && currentIdx >= maxIdx - CHARS_IN_COLUMN) {
            padX = 0;
        }
        if (padX || padY) {
            currentIdx = circulate(currentIdx, padX * CHARS_IN_COLUMN + padY, maxIdx);
            if (currentIdx < baseIdx) {
                baseIdx = currentIdx - currentIdx % CHARS_IN_COLUMN;
            } else if (currentIdx >= baseIdx + CHARS_IN_COLUMN * (COLUMNS_IN_PAGE - 1)) {
                baseIdx = currentIdx - CHARS_IN_COLUMN * (COLUMNS_IN_PAGE - 2)
                        - currentIdx % CHARS_IN_COLUMN; 
            }
            if (!encoder.isEncoding()) playSoundTick();
            isInvalid = true;
        }
    }
    if (encoder.isEncoding() && nextMode == MODE_LIST) {
        bool isSignalOn = encoder.forwardFrame();
        if (isSignalOn != isLastSignalOn) {
            (isSignalOn) ? indicateSignalOn() : indicateSignalOff();
            isLastSignalOn = isSignalOn;
        }
    }
    return nextMode;
}

void drawList(void)
{
    if (isInvalid) {
        arduboy.clear();
        arduboy.printEx(31, 4, F("[CODE LIST]"));
        uint8_t idx = baseIdx;
        uint8_t maxIdx = (isJapanese) ? CHARS_IN_LIST_JA : CHARS_IN_LIST_EN;
        for (uint8_t x = 0; x < COLUMNS_IN_PAGE && idx < maxIdx; x++) {
            for (uint8_t y = 0; y < CHARS_IN_COLUMN && idx < maxIdx; y++) {
                int16_t dx = 10 + x * 56, dy = 12 + y * 6;
                if (idx == currentIdx) {
                    dx -= 4;
                    arduboy.fillRect(dx - 6, dy, 5, 5, WHITE);
                }
                char letter = getLetterInList(idx, isJapanese);
                arduboy.printEx(dx, dy, letter);
                if (x < (COLUMNS_IN_PAGE - 1)) {
                    drawMorseCode(dx + 8, dy + 2, encoder.getMorseCode(letter));
                }
                idx++;
            }
        }
        isInvalid = false;
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static char getLetterInList(uint8_t idx, bool isJapanese)
{
    char ret;
    if (isJapanese) {
        if (idx < CHARS_JAPANESE) {
            ret = 0x60 + idx;
        } else if (idx < CHARS_JAPANESE + CHARS_NUMBER) {
            ret = '0' + (idx - CHARS_JAPANESE + 1) % CHARS_NUMBER;
        } else {
            ret = pgm_read_byte(&extendedListJA[idx - (CHARS_JAPANESE + CHARS_NUMBER)]);
        }
    } else {
        if (idx < CHARS_ALPHABET) {
            ret = 'A' + idx;
        } else if (idx < CHARS_ALPHABET + CHARS_NUMBER) {
            ret = '0' + (idx - CHARS_ALPHABET + 1) % CHARS_NUMBER;
        } else {
            ret = pgm_read_byte(&extendedListEN[idx - (CHARS_ALPHABET + CHARS_NUMBER)]);
        }
    }
    return ret;
}
