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

PROGMEM static const uint8_t imgTitle[192] = { // 24x64
    0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0xFC, 0xFC,
    0xFC, 0x1C, 0x1C, 0x1C, 0x1C, 0xFC, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xE0, 0xFF,
    0xFF, 0x01, 0xFF, 0xFF, 0xFF, 0x00, 0xEF, 0xEF, 0xEF, 0xEE, 0xEE, 0xEE, 0x0E, 0xEF, 0xEF, 0xEF,
    0x00, 0x00, 0x00, 0xFE, 0xFE, 0xFE, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0x00, 0x7F, 0x7F,
    0x7F, 0x70, 0x7F, 0x7F, 0x70, 0x7F, 0x7F, 0x7F, 0x0E, 0x0E, 0x00, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0x00, 0xFF, 0xFF, 0xFF, 0x87, 0xFF, 0xFF, 0x80, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x7F, 0x7F, 0x7F, 0x00, 0x7F, 0x7F, 0x70, 0x7F, 0x7F, 0x7F, 0x00, 0x3B, 0x3B,
    0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0xFB, 0xFB, 0xFB, 0x70, 0x70, 0x70, 0x7F, 0x7F, 0x7F, 0x70, 0x70,
    0x70, 0x70, 0x70, 0x70, 0x70, 0x00, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x80, 0xFF, 0xFF, 0x07, 0xFF, 0xFF, 0xFF, 0x00, 0xEE, 0xEE,
    0xEE, 0x0E, 0xEE, 0xEE, 0xEE, 0xEF, 0xEF, 0xEF, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x00, 0x03, 0x03, 0x03, 0x00, 0x7F, 0x7F, 0x7F, 0x70, 0x7F, 0x7F, 0x00, 0x7F, 0x7F, 0x7F
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
    addMenuItem(F("BEGINNER"), onStart);
    addMenuItem(F("NOVICE"), onStart);
    addMenuItem(F("STANDARD"), onStart);
    addMenuItem(F("VETERAN"), onStart); // GOD-HAND?
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
    int8_t pos = getMenuItemPos();
    if (pos == 3 && arduboy.buttonPressed(LEFT_BUTTON_V)) pos++;
    startLevel = (pos < 2) ? pos * 10 : (pos - 1) * 30; 
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
    arduboy.printEx(41, 63, F("DEBUG"));
#endif
    arduboy.drawBitmap(16, 0, imgTitle, IMG_TITLE_W, IMG_TITLE_H, WHITE);
    arduboy.fillRect(40, 9, 4, 3, WHITE);
    if (lastScore > 0) drawNumberV(0, -1, lastScore, ALIGN_RIGHT);
}

static void drawRecord(void)
{
    arduboy.printEx(4, 55, F("BEST 5"));
    arduboy.printEx(10, 43, F("SCORES"));
    for (int i = 0; i < 5; i++) {
        arduboy.printEx(22 + i * 14, 61, F("[ ]"));
        drawNumberV(22 + i * 14, 55, i + 1, ALIGN_LEFT);
        drawNumberV(22 + i * 14, 1, record.hiscore[i].score, ALIGN_RIGHT);
        drawLabelLevel(28 + i * 14, 13);
        drawNumberV(28 + i * 14, -1, record.hiscore[i].level, ALIGN_RIGHT);
    }
    arduboy.drawRect(19, -1, 73, HEIGHT + 2, WHITE);
    arduboy.printEx(98, 63, F("PLAY COUNT"));
    drawNumberV(104, -1, record.playCount, ALIGN_RIGHT);
    arduboy.printEx(112, 63, F("PLAY TIME"));
    drawTime(118, -1, record.playFrames, ALIGN_RIGHT);
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
