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

/*PROGMEM static const uint8_t imgTitle[512] = { // 128x32
};*/

PROGMEM static const char creditText[] = "- " APP_TITLE " -\0\0" APP_RELEASED \
        "\0PROGREMMED BY OBONO\0\0THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.";

PROGMEM static const byte soundStart[] = {
    0x90, 72, 0, 100, 0x80, 0, 25,
    0x90, 74, 0, 100, 0x80, 0, 25,
    0x90, 76, 0, 100, 0x80, 0, 25,
    0x90, 77, 0, 100, 0x80, 0, 25,
    0x90, 79, 0, 200, 0x80, 0xF0
};

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
    setMenuCoords(44, 34, 84, 30, false, true);
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
    arduboy.playScore2(soundStart, 0);
    state = STATE_STARTED;
    gameMode = GAME_MODE_ENDLESS;
    dprintln(F("Start Endless"));
}

static void onStartLimited(void)
{
    arduboy.playScore2(soundStart, 0);
    state = STATE_STARTED;
    gameMode = GAME_MODE_LIMITED;
    dprintln(F("Start Time Limited"));
}

static void onStartPuzzle(void)
{
#if 0
    arduboy.playScore2(soundStart, 0);
    state = STATE_STARTED;
    gameMode = GAME_MODE_PUZZLE;
    dprintln(F("Start Puzzle"));
#endif
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
    arduboy.printEx(0, 0, F(APP_TITLE));
    arduboy.printEx(0, 6, F("TITLE SCREEN"));

    //arduboy.drawBitmap(0, 0, imgTitle, 128, 32, WHITE);
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

    arduboy.drawBitmap(88, 16, imgLblLevel, 19, 5, WHITE);
    arduboy.drawBitmap(88, 32, imgLblChain, 17, 5, WHITE);

    drawNumber(73, 10, record.endlessHiscore);
    drawNumber(112, 16, record.endlessMaxLevel);
    drawNumber(73, 26, record.limitedHiscore);
    drawNumber(110, 32, record.limitedMaxChain);
    drawNumber(73, 42, record.puzzleClearCount);
    arduboy.print('/');
    arduboy.print((record.puzzleClearCount < 50) ? 50 : 250);
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
