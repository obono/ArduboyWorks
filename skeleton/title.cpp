#include "common.h"

/*  Defines  */

#define CONFIG_MAX  9

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_TOP,
    STATE_SETTINGS,
    STATE_CREDIT,
    STATE_STARTED,
};

/*  Typedefs  */

/*  Local Functions  */

static void handleInit(void);
static void handleTop(void);
static void handleSettings(void);
static void handleAnyButton(void);

static void onTop(void);
static void onStart(void);
static void onSettings(void);
static void onCredit(void);
static void onConfig(void);

static void drawInit(void);
static void drawTop(void);
static void drawSettings(void);
static void drawCredit(void);

static void drawText(const char *p, int lines);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable + n))()

/*  Local Variables  */

PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGRAMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleInit, handleTop, handleSettings, handleAnyButton
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    drawInit, drawTop, drawSettings, drawCredit
};

static STATE_T  state = STATE_INIT;
static bool     isSettingChanged;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
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
    callDrawerFunc(state);
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

static void handleSettings(void)
{
    uint8_t pos = getMenuItemPos();
    if (pos < 4 && padX != 0) {
        playSoundTick();
        record.config[pos] = circulateOne(record.config[pos], padX, CONFIG_MAX);
        isRecordDirty = true;
        isSettingChanged = true;
    }
    handleMenu();
    if (padY) isSettingChanged = true;
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
    if (state == STATE_SETTINGS) {
        playSoundClick();
        writeRecord();
    }
    clearMenuItems();
    addMenuItem(F("GAME START"), onStart);
    addMenuItem(F("SETTINGS"), onSettings);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuItemPos((state == STATE_SETTINGS) ? 1 : 0);
    setMenuCoords(56, 40, 72, 17, false, true);
    state = STATE_TOP;
    isInvalid = true;
}

static void onStart(void)
{
    state = STATE_STARTED;
}

static void onSettings(void)
{
    playSoundClick();
    clearMenuItems();
    addMenuItem(F("CONFIG 1"), onConfig);
    addMenuItem(F("CONFIG 2"), onConfig);
    addMenuItem(F("CONFIG 3"), onConfig);
    addMenuItem(F("CONFIG 4"), onConfig);
    addMenuItem(F("EXIT"), onTop);
    setMenuItemPos(0);
    setMenuCoords(10, 8, 112, 35, false, false);
    state = STATE_SETTINGS;
    isInvalid = true;
    isSettingChanged = true;
}

static void onCredit(void)
{
    playSoundClick();
    state = STATE_CREDIT;
    isInvalid = true;
}

static void onConfig(void)
{
    playSoundClick();
    uint8_t pos = getMenuItemPos();
    record.config[pos] = circulateOne(record.config[pos], 1, CONFIG_MAX);
    isRecordDirty = true;
    isSettingChanged = true;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawInit(void)
{
    ab.clear();
    ab.printEx(counter, 0, F("TITLE SCREEN"));
}

static void drawTop(void)
{
    if (isInvalid) {
        ab.clear();
        ab.printEx(0, 0, F("TITLE SCREEN"));
    }
    drawMenuItems(isInvalid);
}

static void drawSettings(void)
{
    if (isInvalid) {
        ab.clear();
        ab.printEx(34, 0, F("[SETTINGS]"));
        ab.drawFastHLine(0, 44, 128, WHITE);
        ab.printEx(10, 48, F("PLAY COUNT "));
        ab.print(record.playCount);
        ab.printEx(10, 54, F("PLAY TIME"));
        drawTime(76, 54, record.playFrames);
    }
    drawMenuItems(isInvalid);
    if (isSettingChanged) {
        for (int i = 0; i < 4; i++) {
            int16_t dx = 100 - (getMenuItemPos() == i) * 4, dy = i * 6 + 8;
            ab.printEx(dx, dy, record.config[i]);
        }
        isSettingChanged = false;
    }
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
        ab.printEx(64 - len * 3, y, (const __FlashStringHelper *) p);
        p += len + 1;
        y += (len == 0) ? 2 : 6;
    }
}
