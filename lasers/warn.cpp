#include "common.h"

/*  Defines  */

#define Y_MAX   51

/*  Local Variables  */

PROGMEM static const uint8_t imgBButton[7] = {
    0x3E, 0x41, 0x55, 0x55, 0x51, 0x65, 0x3E
};

PROGMEM static const char warningText[] = "! WARNING !\0\0THE SCREEN FLICKERS\0" \
        "VERY RAPIDLY. IT MAY\0CAUSE DISCOMFORT AND\0TRIGGER EPILEPTIC\0SEIZURES. BE CAREFUL.";

static const char *p;
static uint8_t  x, y;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initWarn(void)
{
    p = warningText;
    x = 31;
    y = 9;
    isInvalid = true;
}

MODE_T updateWarn(void)
{
    return (y == Y_MAX && arduboy.buttonDown(B_BUTTON)) ? MODE_TITLE : MODE_WARN;
}

void drawWarn(void)
{
    if (isInvalid) {
        arduboy.clear();
        arduboy.fillRect2(0, 8, WIDTH, 7, WHITE);
        arduboy.setTextColor(BLACK, BLACK);
        isInvalid = false;
    }

    while (y < Y_MAX) {
        char c = pgm_read_byte(p++);
        if (c == '\0') {
            arduboy.setTextColor(WHITE, WHITE);
            x = 4;
            y += 6;
            dprintln();
            if (y == Y_MAX) {
                arduboy.printEx(13, 57, F("PRESS   TO ACCEPT"));
                arduboy.drawBitmap(48, 56, imgBButton, 7, 7, WHITE);
            }
        } else {
            arduboy.setCursor(x, y);
            arduboy.print(c);
            dprint(c);
            x += 6;
            if (c > ' ') break;
        }
    }
}
