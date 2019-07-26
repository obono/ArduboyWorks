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

/*  Typedefs  */

/*  Local Functions  */

static void onStart(void);
static void onRecord(void);
static void onCredit(void);
static void handleAnyButton(void);

static void drawTitleImage(void);
static void drawRecord(void);
static void drawCredit(void);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[192] = { // 24x64
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0x80, 0x80, 0xB0, 0xE0, 0x00, 0x00,
    0xB8, 0xFC, 0x9C, 0x98, 0x38, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xC0, 0xC0, 0x1C,
    0x1F, 0x8F, 0x99, 0x33, 0xE3, 0x81, 0x00, 0x00, 0xE1, 0xF3, 0x73, 0xE3, 0x07, 0xFF, 0x07, 0x0E,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x01, 0x00, 0xE1, 0xE3, 0xE3, 0xC7, 0xCF, 0xFB, 0x00, 0x00,
    0x83, 0xC7, 0xCE, 0x8F, 0x0E, 0xF7, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00,
    0xC3, 0xCF, 0x1C, 0x1C, 0x7F, 0xCF, 0x00, 0x00, 0x8F, 0x9F, 0xB9, 0xBF, 0x38, 0xDF, 0x00, 0x00,
    0xFF, 0xFF, 0x9F, 0x1F, 0x8F, 0x80, 0x03, 0x87, 0xDF, 0xDF, 0xCE, 0x8E, 0x0E, 0xC7, 0x80, 0x00,
    0x99, 0xBB, 0xBB, 0xBB, 0xFF, 0xEF, 0x00, 0x00, 0x01, 0x07, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0xFF,
    0xFF, 0xCF, 0x8F, 0xC7, 0xC0, 0x07, 0xFF, 0xFC, 0x01, 0xE3, 0xE3, 0xE3, 0xE7, 0xC6, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0xF7, 0xF7, 0xF7, 0xFB, 0xFC, 0x7F, 0x9F,
    0xE0, 0x01, 0x01, 0x03, 0x03, 0x07, 0xFF, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x0F, 0x3F, 0x7E, 0xFC, 0xF8, 0xFC, 0x7E, 0x3F, 0x0F
};

PROGMEM static const char creditText[] = "STAIR   \0   SWEEP\0\0\0" APP_RELEASED \
        "\0PROGRAMMED\0BY OBONO\0\0THIS\0PROGRAM IS\0RELEASED\0UNDER THE\0MIT\0 LICENSE.";

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
    arduboy.drawBitmap(16, 0, imgTitle, IMG_TITLE_W, IMG_TITLE_H, WHITE);
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
    const char *p = creditText;
    for (int i = 0; i < 14; i++) {
        uint8_t len = strnlen_P(p, 10);
        arduboy.printEx(i * 6 + 24, HEIGHT / 2 - 1 + len * 3, (const __FlashStringHelper *) p);
        p += len + 1;
    }
    arduboy.drawRect(21, 6, 17, 53, WHITE);
}
