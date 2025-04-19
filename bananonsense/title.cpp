#include "common.h"

/*  Defines  */

enum TITLE_STATE_T : uint8_t {
    STATE_TOP = 0,
    STATE_CREDIT,
    STATE_STARTED,
};

/*  Local Functions  */

static void handleTop(void);
static void handleAnyButton(void);

static void onTop(void);
static void onStart(void);
static void onCredit(void);

static void drawTop(void);
static void drawCredit(void);
static void drawText(const char *p, int16_t y);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable + n))()

/*  Local Constants  */

PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGRAMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleTop, handleAnyButton
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    drawTop, drawCredit
};

/*  Local Variables  */

static TITLE_STATE_T  state = STATE_TOP;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    onTop();
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
    callDrawerFunc(state);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

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
    clearMenuItems();
    addMenuItem(F("GAME START"), onStart);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuItemPos(0);
    setMenuCoords(56, 16, 72, 11, false, true);
    state = STATE_TOP;
    isInvalid = true;
}

static void onStart(void)
{
    state = STATE_STARTED;
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

static void drawTop(void)
{
    if (isInvalid) {
        ab.clear();
        //ab.drawBitmap(0, 0, imgTitle, IMG_TITLE_W, IMG_TITLE_H);
        ab.printEx(0, 58, F("TOTAL"));
        ab.printEx(36, 58, record.banana);
    }
    drawMenuItems(isInvalid);
}

static void drawCredit(void)
{
    if (isInvalid) {
        ab.clear();
        drawText(creditText, 11);
    }
}

/*---------------------------------------------------------------------------*/

static void drawText(const char *p, int16_t y)
{
    while (pgm_read_byte(p) != '\e') {
        uint8_t len = strnlen_P(p, 21);
        ab.printEx(64 - len * 3, y, (const __FlashStringHelper *)p);
        p += len + 1;
        y += (len == 0) ? 2 : 6;
    }
}
