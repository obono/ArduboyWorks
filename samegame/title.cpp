#include "common.h"

/*  Defines  */

#define IMG_TITLE_W     24
#define IMG_TITLE_H     24
#define IMG_SUBTITLE_W  72
#define IMG_SUBTITLE_H  8

#define ONE_HOUR        (60UL * 60UL * FPS)

enum STATE_T {
    STATE_INIT = 0,
    STATE_TITLE,
    STATE_RECORD,
    STATE_CREDIT,
    STATE_START_GAME,
    STATE_START_EDITOR,
};

/*  Local Functions  */

static void onStart(void);
static void onEdit(void);
static void onRecord(void);
static void onCredit(void);
static void handleAnyButton(void);

static void drawTitleImage(void);
static void drawRecord(void);
static void drawCredit(void);
static void drawText(const char *p, int lines);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[][72] = { // 24x24 x3
    {
        0x00, 0x00, 0x00, 0x10, 0x20, 0xC0, 0xC0, 0xC0, 0xC1, 0xC1, 0xC1, 0xC3, 0xE7, 0xFE, 0xF8, 0xF0,
        0xF0, 0x38, 0x38, 0x1C, 0x0C, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x41,
        0x21, 0x11, 0x11, 0x10, 0x10, 0x10, 0x10, 0x31, 0x33, 0x26, 0x7C, 0xF8, 0xF0, 0xE0, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0F, 0x18, 0x38, 0x30, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
        0x70, 0x70, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00
    },{
        0x00, 0x00, 0x08, 0x18, 0x30, 0xE0, 0xC0, 0x40, 0x40, 0x20, 0x20, 0x21, 0xE1, 0xFF, 0x7E, 0x3E,
        0x64, 0x40, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xF0, 0x0C, 0x06, 0x03, 0x1F, 0x3C,
        0xF0, 0xE0, 0xF0, 0xBE, 0x0F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFE, 0xF0, 0x00,
        0x00, 0x03, 0x0F, 0x1C, 0x1C, 0x0C, 0x0E, 0x07, 0x03, 0x01, 0x41, 0x43, 0x41, 0x40, 0x60, 0x20,
        0x30, 0x30, 0x18, 0x0E, 0x0F, 0x03, 0x00, 0x00
    },{
        0xC0, 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0xC2, 0xFE, 0xFE, 0x9C, 0x88, 0x80, 0x80, 0x00, 0x00,
        0x40, 0x84, 0x88, 0x18, 0x31, 0x63, 0x06, 0x0C, 0x03, 0x07, 0x07, 0x83, 0xE3, 0xFB, 0x3F, 0x07,
        0x01, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x38, 0x00, 0x40, 0x40, 0xC3, 0xFE, 0xFC, 0x60, 0x00,
        0x70, 0x78, 0x3E, 0x0F, 0x03, 0x08, 0x08, 0x10, 0x70, 0x70, 0x70, 0x3C, 0x1F, 0x0F, 0x03, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
    }
};

PROGMEM static const uint8_t imgSubTitle[] = { // 72x8
    0x66, 0x8F, 0x99, 0x99, 0xF3, 0x60, 0x00, 0x80, 0xC0, 0xBC, 0x23, 0xBF, 0xF8, 0xC0, 0x80, 0x00,
    0x81, 0xFF, 0x87, 0x3E, 0xF0, 0x30, 0x8E, 0xFF, 0xFF, 0x81, 0x00, 0x81, 0xFF, 0xFF, 0x91, 0xB9,
    0x81, 0xC3, 0x00, 0x00, 0x3C, 0x7E, 0xC3, 0x81, 0x91, 0xF1, 0x76, 0x10, 0x00, 0x80, 0xC0, 0xBC,
    0x23, 0xBF, 0xF8, 0xC0, 0x80, 0x00, 0x81, 0xFF, 0x87, 0x3E, 0xF0, 0x30, 0x8E, 0xFF, 0xFF, 0x81,
    0x00, 0x81, 0xFF, 0xFF, 0x91, 0xB9, 0x81, 0xC3
};


PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGRAMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";

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
    addMenuItem(F("GAME START"), onStart);
    if (record.hiscore[0] >= PERFECT_BONUS || record.playFrames >= ONE_HOUR) {
        addMenuItem(F("DESIGN PATTERN"), onEdit);
    }
    addMenuItem(F("RECORD"), onRecord);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuItemPos((state == STATE_START_EDITOR) ? 1 : 0);
    int8_t h = getMenuItemCount() * 6;
    setMenuCoords(28, HEIGHT - h, 95, h, false, true);
    state = STATE_TITLE;
    isInvalid = true;
}

MODE_T updateTitle(void)
{
    MODE_T ret = MODE_TITLE;
    switch (state) {
    case STATE_TITLE:
        handleMenu();
        if (state == STATE_START_GAME)   ret = MODE_GAME;
        if (state == STATE_START_EDITOR) ret = MODE_EDITOR;
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
        default:
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
    state = STATE_START_GAME;
    dprintln(F("Game start"));
}

static void onEdit(void)
{
    playSoundClick();
    state = STATE_START_EDITOR;
    dprintln(F("Design pattern"));
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
    for (int8_t i = 0; i < 4; i++) {
        const uint8_t *img = imgTitle[(i < 3) ? i : 1];
        arduboy.drawBitmap(15 + i * IMG_TITLE_W, 6, img, IMG_TITLE_W, IMG_TITLE_H, WHITE);
    }
    arduboy.drawBitmap((WIDTH - IMG_SUBTITLE_W) / 2, 30, imgSubTitle,
            IMG_SUBTITLE_W, IMG_SUBTITLE_H, WHITE);
#ifdef DEBUG
    arduboy.printEx(98, 0, F("DEBUG"));
#endif
    if (lastScore > 0) drawNumber(0, 0, lastScore);
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
