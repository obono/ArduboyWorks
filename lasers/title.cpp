#include "common.h"

/*  Defines  */

#define LINES_MAX   8
#define SCALE       16
#define LINE_X_MAX  (WIDTH * SCALE)
#define LINE_VX_MIN (SCALE / 2)
#define LINE_VX_MAX (SCALE * 2)

enum STATE_T {
    STATE_INIT = 0,
    STATE_TITLE,
    STATE_RECORD,
    STATE_CREDIT,
    STATE_STARTED,
};

/*  Typedefs  */

typedef struct {
    int16_t x;
    int8_t  vx;
} LINE_T;

/*  Local Functions  */

static void moveLines(void);
static int8_t getLineVx(void);
static void onStart(void);
static void onRecord(void);
static void onCredit(void);
static void handleAnyButton(void);

static void drawLines(void);
static void drawTitleImage(void);
static void drawRecord(void);
static void drawCredit(void);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[351] = { // 117x24
    0x00, 0x00, 0x00, 0x00, 0xF0, 0xFC, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0x78, 0x3C, 0x1C,
    0x1C, 0xFC, 0xFC, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0x7C,
    0x3C, 0x1E, 0x0E, 0x0F, 0x07, 0x07, 0x07, 0x07, 0x0F, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xF8,
    0xFC, 0xFC, 0x7C, 0x38, 0x38, 0x3C, 0x1C, 0x1C, 0x1C, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x00, 0x00,
    0x00, 0xF8, 0xFC, 0xFC, 0xFC, 0x3C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x3C, 0xFC, 0xF8, 0xF8,
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0x7C, 0x3C, 0x1E, 0x0E, 0x0F, 0x07,
    0x07, 0x07, 0x07, 0x0F, 0x0E, 0x00, 0x80, 0xF0, 0xFF, 0xFF, 0x3F, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xF0, 0xFC, 0x7E, 0x7F, 0x7F, 0x77,
    0x71, 0x30, 0x30, 0x30, 0x38, 0xF8, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
    0x0F, 0x1F, 0x1F, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x3C, 0xFC, 0xFC, 0xF8, 0xF0, 0x00, 0x00, 0x00,
    0x00, 0xE0, 0xFE, 0xFF, 0xFF, 0xEF, 0xE1, 0x60, 0x70, 0x70, 0x30, 0x38, 0x38, 0x18, 0x18, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xE0, 0xFE, 0xFF, 0x7F, 0x77, 0x70, 0x70, 0xF8, 0xF8, 0xFC, 0xDC, 0xCE,
    0x8F, 0x07, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0F, 0x1F, 0x1F, 0x1C, 0x1C,
    0x1C, 0x1C, 0x1C, 0x3C, 0xFC, 0xFC, 0xF8, 0xF0, 0x00, 0x00, 0x7C, 0x7F, 0xFF, 0xFF, 0xE3, 0xE0,
    0x70, 0x70, 0x78, 0x38, 0x38, 0x1C, 0x1C, 0x1E, 0x0E, 0x0E, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0x7F,
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xFC, 0xFF, 0x7F, 0x0F, 0x01, 0x00, 0x00,
    0x00, 0x30, 0x70, 0x70, 0xF0, 0xE0, 0xE0, 0xE0, 0xF0, 0xF0, 0x78, 0x7C, 0x3E, 0x1F, 0x0F, 0x07,
    0x03, 0x00, 0x00, 0x00, 0x70, 0xFF, 0xFF, 0xFF, 0xE3, 0xE0, 0xE0, 0x70, 0x70, 0x78, 0x38, 0x3C,
    0x1C, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFE, 0xFF, 0x7F, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x03, 0x0F, 0x3F, 0xFF, 0xFE, 0xF8, 0x60, 0x00, 0x00, 0x30, 0x70, 0x70, 0xF0,
    0xE0, 0xE0, 0xE0, 0xF0, 0xF0, 0x78, 0x7C, 0x3E, 0x1F, 0x0F, 0x07, 0x03, 0x00, 0x00, 0x00,
};

PROGMEM static const char creditText[] = "- " APP_TITLE " -\0\0" APP_RELEASED \
        "\0PROGREMMED BY OBONO\0\0THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.";

static STATE_T  state = STATE_INIT;
static LINE_T   lines[LINES_MAX]; 

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    if (state == STATE_INIT) {
        readRecord();
        lastScore = 0;
    }

    for (LINE_T *p = &lines[0]; p < &lines[LINES_MAX]; p++) {
        p->x = random(LINE_X_MAX);
        p->vx = getLineVx() * (random(2) * 2 - 1);
    }

    clearMenuItems();
    addMenuItem(F("START GAME"), onStart);
    addMenuItem(F("RECORD"), onRecord);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuCoords(25, 42, 77, 19);
    setMenuItemPos(0);

    state = STATE_TITLE;
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
    moveLines();
    randomSeed(rand() ^ micros()); // Shuffle random
    return ret;
}

void drawTitle(void)
{
    clearScreenGray();
    drawLines();
    switch (state) {
    case STATE_TITLE:
    case STATE_STARTED:
        drawTitleImage();
        drawMenuItems();
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

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void moveLines(void)
{
    for (LINE_T *p = &lines[0]; p < &lines[LINES_MAX]; p++) {
        p->x += p->vx;
        if (p->x < 0) {
            p->x = 0;
            p->vx = getLineVx();
        }
        if (p->x >= LINE_X_MAX) {
            p->x = LINE_X_MAX - 1;
            p->vx = -getLineVx();
        }
    }
}

static int8_t getLineVx(void) {
    return random(LINE_VX_MIN, LINE_VX_MAX + 1);
}

static void onStart(void)
{
    state = STATE_STARTED;
    dprintln(F("Start Game"));
}

static void onRecord(void)
{
    playSoundClick();
    state = STATE_RECORD;
    dprintln(F("Show record"));
}

static void onCredit(void)
{
    playSoundClick();
    state = STATE_CREDIT;
    dprintln(F("Show credit"));
}

static void handleAnyButton(void)
{
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        playSoundClick();
        state = STATE_TITLE;
    }
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawLines(void)
{
    for (int i = 0; i < LINES_MAX; i++) {
        LINE_T *p = &lines[i];
        arduboy.drawFastVLine(p->x / SCALE, 0, HEIGHT, i & 1);
    }
}

static void drawTitleImage(void)
{
#ifdef DEBUG
    arduboy.printEx(98, 0, F("DEBUG"));
#endif
    drawBitmapBW(5, 8, imgTitle, 117, 24);
    if (lastScore > 0) drawNumber(0, 0, lastScore);
}

static void drawRecord(void)
{
    drawFrame(2, 4, 123, 55);
    arduboy.fillRect(3, 5, 121, 8, WHITE);
    arduboy.drawFastHLine2(2, 44, 123, WHITE);
    arduboy.setTextColor(BLACK, BLACK);
    arduboy.printEx(22, 6, F("BEST 10 SCORES"));
    arduboy.setTextColor(WHITE, WHITE);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 5; j++) {
            int r = i * 5 + j;
            arduboy.printEx(i * 60 + 4 - (r == 9) * 6, j * 6 + 14, F("["));
            arduboy.print(r + 1);
            arduboy.print(F("] "));
            arduboy.print(record.hiscore[r]);
        }
    }
    arduboy.printEx(16, 46, F("PLAY COUNT"));
    drawNumber(82, 46, record.playCount);
    arduboy.printEx(16, 52, F("PLAY TIME"));
    drawTime(82, 52, record.playFrames);
}

static void drawCredit(void)
{
    drawFrame(3, 6, 121, 51);
    const char *p = creditText;
    for (int i = 0; i < 8; i++) {
        uint8_t len = strnlen_P(p, 20);
        arduboy.printEx(64 - len * 3, i * 6 + 8, (const __FlashStringHelper *) p);
        p += len + 1;
    }
}
