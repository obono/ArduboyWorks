#include "common.h"

/*  Defines  */

enum STATE_T {
    STATE_INIT = 0,
    STATE_TITLE,
    STATE_RECORD,
    STATE_CREDIT,
    STATE_STARTED,
};

/*  Local Functions  */

static void onStartEndless(void);
static void onStartLimited(void);
static void onStartPuzzle(void);
static void onRecord(void);
static void onCredit(void);
static void handleAnyButton(void);

static void drawTitleImage(void);
static void drawRecord(void);
static void drawCredit(void);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[236] = { // 118x16
    0xF0, 0x28, 0x44, 0x44, 0x82, 0x02, 0x11, 0x11, 0x82, 0x82, 0xC4, 0xC4, 0xE8, 0xE0, 0x00, 0x00,
    0xF0, 0x28, 0x44, 0x44, 0x82, 0x02, 0x11, 0x11, 0x82, 0x82, 0xC4, 0xC4, 0x68, 0x60, 0x00, 0x00,
    0xF0, 0x28, 0x44, 0x44, 0x82, 0x82, 0xFF, 0x00, 0x00, 0x86, 0xC3, 0x00, 0x00, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x28, 0x44, 0x44, 0x82, 0x02, 0x11, 0x11,
    0x82, 0x82, 0xC4, 0xC4, 0x68, 0x60, 0x00, 0x00, 0xF0, 0x28, 0x44, 0x44, 0x82, 0x02, 0x11, 0x11,
    0x82, 0x82, 0xC4, 0xC4, 0xE8, 0xE0, 0x00, 0x00, 0xF0, 0x28, 0x44, 0x44, 0x82, 0x82, 0xFF, 0x00,
    0x00, 0xFE, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xF0, 0x28, 0x44, 0x44, 0x82, 0x02, 0x11, 0x11,
    0x82, 0x82, 0xC4, 0xC4, 0xE8, 0xE0, 0x1F, 0x10, 0x20, 0x20, 0x40, 0x00, 0xFF, 0xFF, 0x19, 0x19,
    0x0C, 0x0C, 0x07, 0x07, 0x00, 0x00, 0x1F, 0x10, 0x20, 0x20, 0x40, 0x00, 0xDF, 0xDF, 0x6D, 0x6D,
    0x36, 0x36, 0x1F, 0x1F, 0x00, 0x00, 0x1F, 0x10, 0x20, 0x20, 0x40, 0x40, 0xFF, 0x80, 0x00, 0x7F,
    0x3F, 0x00, 0x10, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x10,
    0x20, 0x20, 0x40, 0x00, 0xFF, 0xFF, 0x61, 0x61, 0x30, 0x30, 0x18, 0x18, 0x00, 0x00, 0x1F, 0x10,
    0x20, 0x20, 0x40, 0x00, 0xFF, 0xFF, 0x61, 0x61, 0x30, 0x30, 0x1F, 0x1F, 0x00, 0x00, 0x1F, 0x10,
    0x20, 0x20, 0x40, 0x40, 0xFF, 0x80, 0x00, 0x7F, 0x3F, 0x00, 0x10, 0x1F, 0x00, 0x00, 0x1F, 0x10,
    0x20, 0x20, 0x40, 0x00, 0xFF, 0xFF, 0x61, 0x61, 0x30, 0x30, 0x1F, 0x1F,
};

PROGMEM static const uint8_t imgTitlePart[8] = { // 8x8
    0xC0, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0xF8,
};

PROGMEM static const char creditText[] = "- " APP_TITLE " -\0\0" APP_RELEASED \
        "\0PROGREMMED BY OBONO\0\0THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.";

static STATE_T  state = STATE_INIT;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    if (state == STATE_INIT) {
        readRecord();
    }

    clearMenuItems();
    addMenuItem(F("ENDLESS"), onStartEndless);
    addMenuItem(F("TIME LIMITED"), onStartLimited);
    addMenuItem(F("PUZZLE"), onStartPuzzle);
    addMenuItem(F("RECORD"), onRecord);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuCoords(22, 34, 84, 30, false, true);
    setMenuItemPos(gameMode);

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
        case STATE_RECORD:
            drawRecord();
            break;
        case STATE_CREDIT:
            drawCredit();
            break;
        default:
            drawTitleImage();
        }
    }
    if (state == STATE_TITLE || state == STATE_STARTED) {
        drawMenuItems(isInvalid);
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void onStartEndless(void)
{
    state = STATE_STARTED;
    gameMode = GAME_MODE_ENDLESS;
    dprintln(F("Start Endless"));
}

static void onStartLimited(void)
{
    state = STATE_STARTED;
    gameMode = GAME_MODE_LIMITED;
    dprintln(F("Start Time Limited"));
}

static void onStartPuzzle(void)
{
    playSoundClick();
    state = STATE_STARTED;
    gameMode = GAME_MODE_PUZZLE;
    dprintln(F("Start Puzzle"));
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
    arduboy.printEx(0, 0, F(APP_TITLE " DEBUG"));
#else
    arduboy.drawBitmap(5, 8, imgTitle, 118, 16, WHITE);
    arduboy.drawBitmap(43, 0, imgTitlePart, 8, 8, WHITE);
    arduboy.drawBitmap(99, 0, imgTitlePart, 8, 8, WHITE);
#endif
}

static void drawRecord(void)
{
    arduboy.printEx(34, 0, F("[ RECORD ]"));
    arduboy.printEx(16, 10, F("ENDLESS"));
    arduboy.printEx(16, 23, F("TIME"));
    arduboy.printEx(22, 29, F("LIMITED"));
    arduboy.printEx(16, 42, F("PUZZLE"));
    arduboy.printEx(1, 52, F("PLAY TIME"));
    arduboy.printEx(1, 58, F("ERASED DICE"));

    arduboy.drawBitmap(88, 16, imgLabelLevel, 19, 5, WHITE);
    arduboy.drawBitmap(88, 32, imgLabelChain, 17, 5, WHITE);

    drawNumber(73, 10, record.endlessHiscore);
    drawNumber(112, 16, record.endlessMaxLevel);
    drawNumber(73, 26, record.limitedHiscore);
    drawNumber(110, 32, record.limitedMaxChain);
    drawNumber(73, 42, record.puzzleClearCount);
    arduboy.print(F("/40"));
    drawTime(73, 52, record.playFrames);
    drawNumber(73, 58, record.erasedDice);
 
    arduboy.drawRect(-1, 6, 130, 45, WHITE);
}

static void drawCredit(void)
{
    const char *p = creditText;
    for (int i = 0; i < 8; i++) {
        uint8_t len = strnlen_P(p, 20);
        arduboy.printEx(64 - len * 3, i * 6 + 8, (const __FlashStringHelper *) p);
        p += len + 1;
    }
}
