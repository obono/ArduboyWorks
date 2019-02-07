#include "common.h"

/*  Defines  */

enum STATE_T {
    STATE_INIT = 0,
    STATE_PLAYING,
    STATE_MENU,
    STATE_OVER,
    STATE_LEAVE,
};

enum {
    SND_PRIO_START = 0,
};

/*  Typedefs  */

/*  Local Functions  */

static void clearScreen(bool isBlink);

/*  Local Variables  */

PROGMEM static const byte soundStart[] = {
    0x90, 72, 0, 100, 0x90, 74, 0, 100, 0x90, 76, 0, 100,
    0x90, 77, 0, 100, 0x90, 79, 0, 200, 0x80, 0xF0
};

static STATE_T  state = STATE_INIT;
static double   degB, degW;
static bool     blinkFlg;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    record.playCount++;
    degB = 0.0;
    degW = 0.0;
    blinkFlg = true;
    state = STATE_PLAYING;
    arduboy.playScore2(soundStart, 0);
}

MODE_T updateGame(void)
{
    record.playFrames++;
    blinkFlg = !blinkFlg;
    switch (state) {
    case STATE_PLAYING:
        degB += 0.01;
        degW += 0.015;
        if (arduboy.buttonDown(B_BUTTON)) state = STATE_LEAVE;
        break;
    case STATE_MENU:
        handleMenu();
        break;
    default:
        break;
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    clearScreen(blinkFlg);
    if (state == STATE_LEAVE) return;

    arduboy.drawLine(32, 32, 32 + cos(degB) * 48, 32 - sin(degB) * 48, BLACK);
    arduboy.drawLine(96, 32, 96 - cos(degW) * 48, 32 - sin(degW) * 48, WHITE);
    arduboy.printEx(12, 12, F("ABCDEFGHIJ"));
    arduboy.setTextColor(BLACK, BLACK);
    arduboy.printEx(64, 46, F("0123456789"));
    arduboy.setTextColor(WHITE, WHITE);
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void clearScreen(bool isBlink)
{
    uint8_t *sBuffer = arduboy.getBuffer();
    uint8_t b1 = (isBlink) ? 0x55 : 0xaa;
    uint8_t b2 = ~b1;
    asm volatile (
        // load sBuffer pointer into Z
        "movw r30, %0\n\t"
        // counter = 0
        "clr __tmp_reg__ \n\t"
        "loopto: \n\t"
        // (4x) push byte data into screen buffer,
        // then increment buffer position
        "st Z+, %1 \n\t"
        "st Z+, %2 \n\t"
        "st Z+, %1 \n\t"
        "st Z+, %2 \n\t"
        // increase counter
        "inc __tmp_reg__ \n\t"
        // repeat for 256 loops
        // (until counter rolls over back to 0)
        "brne loopto \n\t"
        // input: sBuffer, b1, b2
        // modified: Z (r30, r31)
        :
        : "r" (sBuffer), "r" (b1), "r" (b2)
    );
}