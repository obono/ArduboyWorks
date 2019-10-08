#include "common.h"

/*  Defines  */

#define IMG_TITLE_W 24
#define IMG_TITLE_H 64

enum STATE_T {
    STATE_INIT = 0,
    STATE_TITLE,
    STATE_RECORD,
    STATE_CREDIT,
    STATE_STARTED,
};

#define APP_TITLE_V     "STAIRS  \0   SWEEP"
#define APP_RELEASED_V  "NOVEMBER\0" "2019"

/*  Typedefs  */

/*  Local Functions  */

static void onStart(void);
static void onRecord(void);
static void onCredit(void);
static void handleAnyButton(void);

static void drawTitleImage(void);
static void drawRecord(void);
static void drawCredit(void);
static void drawText(const char *p, int16_t x);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[] = { // 24x64
    0x00
};

PROGMEM static const char creditText[] = APP_TITLE_V "\0 \0" APP_RELEASED_V "\0\0" \
        "PROGRAMMED\0BY OBONO\0\0THIS\0PROGRAM IS\0RELEASED\0UNDER THE\0MIT\0 LICENSE.\0\e";

static STATE_T  state = STATE_INIT;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    if (state == STATE_INIT) {
        readRecord();
        lastScore = 0;
    }

    clearMenuItems();
    addMenuItem(F("NOVICE"), onStart);
    addMenuItem(F("STANDARD"), onStart);
    addMenuItem(F("EXPERT"), onStart);
    addMenuItem(F("GOD-HAND"), onStart);
    addMenuItem(NULL, NULL);
    addMenuItem(F("RECORD"), onRecord);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuCoords(70, 63, 41, 64, false, true);
    setMenuItemPos(0);

    state = STATE_TITLE;
    isInvalid = true;
}

MODE_T updateTitle(void)
{
    MODE_T ret = MODE_TITLE;
    if (state == STATE_TITLE) {
        handleMenu();
        if (state == STATE_STARTED) {
            ret = MODE_GAME;
        }
    } else {
        handleAnyButton();
    }
    randomSeed(rand() ^ micros()); // Shuffle random
    return ret;
}

void drawTitle(void)
{
    if (isInvalid) {
        arduboy.clear();
        switch (state) {
        case STATE_TITLE:
        case STATE_STARTED:
            drawTitleImage();
            break;
        case STATE_RECORD:
            drawRecord();
            break;
        case STATE_CREDIT:
            drawCredit();
            break;
        default:
            break;
        }
    }
    if (state == STATE_TITLE) {
        drawMenuItems(isInvalid);
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void onStart(void)
{
    state = STATE_STARTED;
    isInvalid = true;
    dprintln(F("Start Game"));
}

static void onRecord(void)
{
    playSoundClick();
    state = STATE_RECORD;
    isInvalid = true;
    dprintln(F("Show record"));
}

static void onCredit(void)
{
    playSoundClick();
    state = STATE_CREDIT;
    isInvalid = true;
    dprintln(F("Show credit"));
}

static void handleAnyButton(void)
{
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        playSoundClick();
        state = STATE_TITLE;
        isInvalid = true;
    }
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawTitleImage(void)
{
#ifdef DEBUG
    arduboy.printEx(98, 0, F("DEBUG"));
#endif
    //arduboy.drawBitmap(16, 0, imgTitle, IMG_TITLE_W, IMG_TITLE_H, WHITE);
    arduboy.printEx(12, 63, F("STAIRS\n    SWEEP\n\nTITLE\n    SCREEN"));
    if (lastScore > 0) drawNumber(0, 63, 0, lastScore);
}

static void drawRecord(void)
{
    arduboy.printEx(8, 52, F("BEST 10"));
    arduboy.printEx(14, 49, F("SCORES"));
    for (int i = 0; i < 10; i++) {
        drawNumber(26 + i * 6, 63, 2, i + 1);
        drawNumber(26 + i * 6, 47, 8, record.hiscore[i]);
    }
    arduboy.drawRect(23, -1, 65, HEIGHT + 2, WHITE);
    arduboy.printEx(96, 61, F("PLAY COUNT"));
    drawNumber(102, 61, 10, record.playCount);
    arduboy.printEx(108, 61, F("PLAY TIME"));
    drawTime(114, 43, record.playFrames);
}

static void drawCredit(void)
{
    drawText(creditText, 24);
    arduboy.drawRect(21, 6, 17, 53, WHITE);
}

static void drawText(const char *p, int16_t x)
{
    while (pgm_read_byte(p) != '\e') {
        uint8_t len = strnlen_P(p, 21);
        arduboy.printEx(x, HEIGHT / 2 - 1 + len * 3, (const __FlashStringHelper *) p);
        p += len + 1;
        x += (len == 0) ? 2 : 6;
    }
}
