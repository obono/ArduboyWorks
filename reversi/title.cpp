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

static void onStartBlack(void);
static void onStartWhite(void);
static void onStart2Players(void);
static void onRecord(void);
static void onCredit(void);
static void handleAnyButton(void);

static void drawTitleImage(void);
static void drawRecord(void);
static void drawCredit(void);

/*  Local Variables  */

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
    addMenuItem(F("VS CPU AS BLACK"), onStartBlack);
    addMenuItem(F("VS CPU AS WHITE"), onStartWhite);
    addMenuItem(F("2 PLAYERS"), onStart2Players);
    addMenuItem(F("RECORD"), onRecord);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuCoords(22, 34, 106, 30, false, true);
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

static void onStartBlack(void)
{
    state = STATE_STARTED;
    gameMode = GAME_MODE_BLACK;
    dprintln(F("Start game as Black"));
}

static void onStartWhite(void)
{
    state = STATE_STARTED;
    gameMode = GAME_MODE_WHITE;
    dprintln(F("Start game as White"));
}

static void onStart2Players(void)
{
    state = STATE_STARTED;
    gameMode = GAME_MODE_2PLAYERS;
    dprintln(F("Start 2 players game"));
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
}

static void drawRecord(void)
{
    arduboy.printEx(22, 4, F("BEST 10 SCORES"));
    arduboy.drawFastHLine2(0, 12, 128, WHITE);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 5; j++) {
            int r = i * 5 + j;
            arduboy.printEx(i * 60 + 4 - (r == 9) * 6, j * 6 + 14, F("["));
            arduboy.print(r + 1);
            arduboy.print(F("] "));
            arduboy.print(record.hiscore[r]);
        }
    }
    arduboy.drawFastHLine2(0, 44, 128, WHITE);
    arduboy.printEx(16, 48, F("PLAY COUNT "));
    arduboy.print(record.playCount);
    arduboy.printEx(16, 54, F("PLAY TIME"));
    drawTime(82, 54, record.playFrames);
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
