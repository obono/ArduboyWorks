#include "common.h"

/*  Local Constants  */

PROGMEM static const char creditText[] = \
        "[" APP_TITLE "]\0\0\0" APP_RELEASED "\0PROGRAMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initCredit(void)
{
    isInvalid = true;
}

MODE_T updateCredit(void)
{
    MODE_T ret = MODE_CREDIT;
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        playSoundClick();
        ret = MODE_CONSOLE;
    }
    return ret;
}

void drawCredit(void)
{
    if (isInvalid) {
        arduboy.clear();
        int16_t y = 11;
        const char *p = creditText;
        while (pgm_read_byte(p) != '\e') {
            uint8_t len = strnlen_P(p, 21);
            arduboy.printEx(64 - len * 3, y, (const __FlashStringHelper *) p);
            p += len + 1;
            y += (len == 0) ? 2 : 6;
        }
        isInvalid = false;
    }
}
