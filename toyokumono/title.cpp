#include "common.h"

/*  Defines  */

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_TOP,
    STATE_RECORD,
    STATE_CREDIT,
    STATE_STARTED,
};

#define IMG_TITLE_W 72
#define IMG_TITLE_H 24

/*  Typedefs  */

/*  Local Functions  */

static void handleInit(void);
static void handleTop(void);
static void handleAnyButton(void);

static void onTop(void);
static void onStart(void);
static void onRecord(void);
static void onCredit(void);

static void drawInit(void);
static void drawTop(void);
static void drawRecord(void);
static void drawCredit(void);

static void drawText(const char *p, int lines);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable + n))()

/*  Local Constants  */

PROGMEM static const uint8_t imgTitle[216] = { // 72x24
    0x00, 0xFC, 0xFC, 0xFC, 0x6C, 0x6C, 0x6C, 0xFE, 0xFE, 0xFE, 0x6C, 0x6C, 0x6C, 0x6C, 0xFE, 0xFE,
    0xFE, 0x6C, 0x6C, 0x6C, 0xFC, 0xFC, 0xFC, 0x00, 0x00, 0xF0, 0xF6, 0xF6, 0x36, 0xB6, 0xB6, 0xB6,
    0xB6, 0x36, 0xFE, 0xFE, 0xFE, 0xFE, 0x36, 0xB6, 0xB6, 0xB6, 0xB6, 0x36, 0xF6, 0xF6, 0xF0, 0x00,
    0x00, 0xFE, 0xFE, 0xFE, 0xC6, 0xFE, 0xFE, 0xFE, 0xC6, 0xFE, 0xFE, 0xFE, 0x00, 0x06, 0x06, 0x86,
    0xC6, 0xE6, 0x76, 0x3E, 0x1E, 0x0E, 0x06, 0x00, 0x00, 0x1B, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB,
    0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0x1B, 0x00,
    0x00, 0x0F, 0x6F, 0x6F, 0x60, 0x6D, 0x6D, 0x6D, 0x6D, 0x60, 0x6F, 0x6F, 0x6F, 0x6F, 0x60, 0x6D,
    0x6D, 0x6D, 0x6D, 0x60, 0x6F, 0x6F, 0x0F, 0x00, 0x00, 0x9F, 0x9F, 0x9F, 0x98, 0xFF, 0xFF, 0xFF,
    0x98, 0x9F, 0x9F, 0x9F, 0x00, 0x03, 0x03, 0xC3, 0xE3, 0xF3, 0x3B, 0x1F, 0x0F, 0x07, 0x03, 0x00,
    0x00, 0x30, 0x37, 0x37, 0x37, 0x3F, 0x3E, 0x3E, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x3E, 0x3E, 0x3F, 0x37, 0x37, 0x37, 0x30, 0x00, 0x00, 0x03, 0x33, 0x3B, 0x3F, 0x3F, 0x3F, 0x37,
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x37, 0x3F, 0x3F, 0x3F, 0x3B, 0x33, 0x03, 0x00,
    0x00, 0x31, 0x31, 0x31, 0x31, 0x3F, 0x3F, 0x3F, 0x31, 0x31, 0x31, 0x31, 0x00, 0x30, 0x30, 0x3F,
    0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGRAMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleInit, handleTop, handleAnyButton, handleAnyButton
};

/*  Local Variables  */


PROGMEM static void(*const drawerFuncTable[])(void) = {
    drawInit, drawTop, drawRecord, drawCredit
};

static STATE_T  state = STATE_INIT;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    clearMenuItems();
    addMenuItem(F("GAME START"), onStart);
    addMenuItem(F("RECORD"), onRecord);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuItemPos(0);
    setMenuCoords(56, 40, 72, 17, false, true);
    if (state == STATE_INIT) {
        counter = FPS;
    } else {
        onTop();
    }
}

MODE_T updateTitle(void)
{
    callHandlerFunc(state);
    randomSeed(rand() ^ micros()); // Shuffle random
    return (state == STATE_STARTED) ? MODE_GAME : MODE_TITLE;
}

void drawTitle(void)
{
    if (state == STATE_STARTED) return;
    if (isInvalid) {
        ab.clear();
        callDrawerFunc(state);
    }
    if (state == STATE_TOP) drawMenuItems(isInvalid);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handleInit(void)
{
    if (--counter == 0) onTop();
    isInvalid = true;
}

static void handleTop(void)
{
    handleMenu();
}

static void handleAnyButton(void)
{
    if (ab.buttonDown(A_BUTTON | B_BUTTON)) {
        playSoundClick();
        state = STATE_TOP;
        isInvalid = true;
    }
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onTop(void)
{
    state = STATE_TOP;
    isInvalid = true;
}

static void onStart(void)
{
    state = STATE_STARTED;
}

static void onRecord(void)
{
    playSoundClick();
    state = STATE_RECORD;
    isInvalid = true;
}

static void onCredit(void)
{
    playSoundClick();
    state = STATE_CREDIT;
    isInvalid = true;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawInit(void)
{
    ab.drawBitmap(28, -counter / 2, imgTitle, IMG_TITLE_W, IMG_TITLE_H, WHITE);
    ab.printEx(7, IMG_TITLE_H + counter, F("T O Y O K U M O N O"));
}

static void drawTop(void)
{
    ab.drawBitmap(28, 0, imgTitle, IMG_TITLE_W, IMG_TITLE_H, WHITE);
    ab.printEx(7, IMG_TITLE_H, F("T O Y O K U M O N O"));
    if (lastScore > 0) ab.printEx(0, 0, lastScore);
}

static void drawRecord(void)
{
    ab.printEx(22, 4, F("BEST 10 SCORES"));
    ab.drawFastHLine(0, 12, 128, WHITE);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 5; j++) {
            int r = i * 5 + j;
            ab.printEx(i * 60 + 4 - (r == 9) * 6, j * 6 + 14, F("["));
            ab.print(r + 1);
            ab.print(F("] "));
            ab.print(record.hiscore[r]);
        }
    }
    ab.drawFastHLine(0, 44, 128, WHITE);
    ab.printEx(16, 48, F("PLAY COUNT "));
    ab.print(record.playCount);
    ab.printEx(16, 54, F("PLAY TIME"));
    drawTime(82, 54, record.playFrames);
}

static void drawCredit(void)
{
    drawText(creditText, 11);
}

/*---------------------------------------------------------------------------*/

static void drawText(const char *p, int16_t y)
{
    while (pgm_read_byte(p) != '\e') {
        uint8_t len = strnlen_P(p, 21);
        ab.printEx(64 - len * 3, y, (const __FlashStringHelper *) p);
        p += len + 1;
        y += (len == 0) ? 2 : 6;
    }
}
