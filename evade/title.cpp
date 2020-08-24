#include "common.h"

/*  Defines  */

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_TITLE,
    STATE_RECORD,
    STATE_CREDIT,
    STATE_STARTED,
};

/*  Local Functions  */

static void initTitleMenu(bool isFromSettings);
static void onStart(void);
static void onRecord(void);
static void onCredit(void);
static void handleAnyButton(void);

static void drawTitleImage(void);
static void drawRecord(void);
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
        arduboy.initAudio(1); // TODO: 1 or 2
        readRecord();
    }
    clearMenuItems();
    addMenuItem(F("GAME START"), onStart);
    addMenuItem(F("SETTINGS"), onRecord);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuItemPos(0);
    setMenuCoords(22, 46, 83, 18, false, true);
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
        case STATE_RECORD:
            drawRecord();
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
    state = STATE_STARTED;
    dprintln(F("Game start"));
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
    //arduboy.drawBitmap(0, 0, imgTitle, IMG_TITLE_W, IMG_TITLE_H, WHITE);
    arduboy.printEx(16, 8, F(APP_TITLE));
    arduboy.printEx(16, 14, F("TITLE SCREEN"));
#ifdef DEBUG
    arduboy.printEx(16, 20, F("DEBUG"));
#endif
    if (lastScore > 0) arduboy.printEx(0, 0, lastScore);
}

static void drawRecord(void)
{
    /*arduboy.printEx(22, 4, F("BEST 10 SCORES"));
    arduboy.drawFastHLine(0, 12, 128, WHITE);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 5; j++) {
            int r = i * 5 + j;
            arduboy.printEx(i * 60 + 4 - (r == 9) * 6, j * 6 + 14, F("["));
            arduboy.print(r + 1);
            arduboy.print(F("] "));
            arduboy.print(record.hiscore[r]);
        }
    }*/
    arduboy.drawFastHLine(0, 44, 128, WHITE);
    arduboy.printEx(10, 48, F("PLAY COUNT "));
    arduboy.print(record.playCount);
    arduboy.printEx(10, 54, F("PLAY TIME"));
    drawTime(76, 54, record.playFrames);
}

static void drawCredit(void)
{
    drawText(creditText, 11);
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
