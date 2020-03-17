#include "common.h"

/*  Defines  */

enum STATE_T {
    STATE_INIT = 0,
    STATE_TITLE,
    STATE_CREDIT,
    STATE_STARTED,
};

/*  Local Functions  */

static void initTitleMenu(bool isFromSettings);
static void onStart(void);
static void onCredit(void);
static void handleAnyButton(void);

static void drawTitleImage(void);
static void drawCredit(void);
static void drawText(const char *p, int lines);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[] = { // TODO
};

PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGREMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";

static STATE_T  state = STATE_INIT;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    if (state == STATE_INIT) {
        arduboy.initAudio(1);
        readRecord();
    }
    clearMenuItems();
    addMenuItem(F("RETRY"), onStart);
    addMenuItem(NULL, NULL);
    addMenuItem(F("NEW BATTLE"), NULL);
    addMenuItem(F(" RANDOM"), onStart);
    addMenuItem(F(" SPECIFY CODE"), onStart);
    addMenuItem(NULL, NULL);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuItemPos((record.playCount == 0) ? 3 : 0);
    setMenuCoords(38, 28, 90, 34, false, true);
    state = STATE_TITLE;
    isInvalid = true;
}

MODE_T updateTitle(void)
{
    MODE_T ret = MODE_TITLE;
    switch (state) {
    case STATE_TITLE:
        handleMenu();
        if (state == STATE_STARTED) ret = MODE_GAME;
        break;
    default:
        handleAnyButton();
        break;
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
            drawTitleImage();
            break;
        case STATE_CREDIT:
            drawCredit();
            break;
        }
    }
    if (state == STATE_TITLE) drawMenuItems(isInvalid);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void onStart(void)
{
    if (getMenuItemPos() != 0 || record.playCount == 0) record.gameSeed = random(); // TODO
    state = STATE_STARTED;
    dprintln(F("Game start"));
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
    //arduboy.drawBitmap(0, 0, imgTitle, IMG_TITLE_W, IMG_TITLE_H, WHITE);
    arduboy.printEx(16, 8, F(APP_TITLE));
    arduboy.printEx(16, 14, F("TITLE SCREEN"));
#ifdef DEBUG
    arduboy.printEx(16, 20, F("DEBUG"));
#endif
}

static void drawCredit(void)
{
    drawText(creditText, 3);
    arduboy.drawFastHLine(0, 47, 128, WHITE);
    arduboy.printEx(10, 51, F("PLAY COUNT "));
    arduboy.print(record.playCount);
    arduboy.printEx(10, 57, F("PLAY TIME"));
    drawTime(76, 57, record.playFrames);
}

static void drawText(const char *p, int16_t y)
{
    while (pgm_read_byte(p) != '\e') {
        uint8_t len = strnlen_P(p, 21);
        arduboy.printEx(64 - len * 3, y, (const __FlashStringHelper *) p);
        p += len + 1;
        y += (len == 0) ? 2 : 6;
    }
}
