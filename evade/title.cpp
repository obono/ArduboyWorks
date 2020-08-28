#include "common.h"

/*  Defines  */

#define SPEED_MAX       9
#define ACCEL_MAX       3
#define DENSITY_MAX     5
#define THICKNESS_MAX   5

enum STATE_T : uint8_t {
    STATE_INIT = 0,
    STATE_TOP,
    STATE_SETTINGS,
    STATE_CREDIT,
    STATE_STARTED,
};

/*  Local Functions  */

static void handleTop(void);
static void handleSettings(void);
static void handleAnyButton(void);

static void onTop(void);
static void onStart(void);
static void onSettings(void);
static void onCredit(void);
static void onMenuSpeed(void);
static void onMenuAccel(void);
static void onMenuDensity(void);
static void onMenuThickness(void);
static void onMenuSimpleMode(void);

static void drawTop(void);
static void drawSettings(void);
static void drawCredit(void);
static void drawText(const char *p, int lines);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable + n))()

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[] = { // TODO
};

PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGREMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";
PROGMEM static const char accelText[] = "OFF \0SLOW\0FAST";

PROGMEM static void(*const handlerFuncTable[])(void) = {
    NULL, handleTop, handleSettings, handleAnyButton
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    NULL, drawTop, drawSettings, drawCredit
};

static STATE_T  state = STATE_INIT;
static bool     isSettingChenged;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    if (state == STATE_INIT) {
        // do nothing?
    }
    if (record.simpleMode && state == STATE_STARTED) {
        onSettings();
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
    if (isInvalid) arduboy.clear();
    callDrawerFunc(state);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handleTop(void)
{
    if (record.simpleMode) {
        SIMPLE_OP_T op = handleSimpleMode();
        if (op == SIMPLE_OP_START) onStart();
        if (op == SIMPLE_OP_SETTINGS) onSettings();
    } else {
        handleMenu();
    }
}

static void handleSettings(void)
{
    handleDPad();
    uint8_t pos = getMenuItemPos();
    if (pos <= 3) {
        if (padX != 0) {
            playSoundTick();
            switch (pos) {
            case 0: record.speed = circulateOne(record.speed, padX, SPEED_MAX); break;
            case 1: record.acceleration = circulate(record.acceleration, padX, ACCEL_MAX); break;
            case 2: record.density = circulateOne(record.density, padX, DENSITY_MAX); break;
            case 3: record.thickness = circulateOne(record.thickness, padX, THICKNESS_MAX); break;
            }
            isSettingChenged = true;
        }
    } else if (pos == 4 && arduboy.buttonDown(LEFT_BUTTON | RIGHT_BUTTON)) {
        onMenuSimpleMode();
    }
    handleMenu();
    if (isSettingChenged) {
        record.hiscore = 0;
        isRecordDirty = true;
    }
    if (arduboy.buttonDown(UP_BUTTON | DOWN_BUTTON)) isSettingChenged = true;
}

static void handleAnyButton(void)
{
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
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
        writeRecord();
        playSoundClick();
    }
    clearMenuItems();
    addMenuItem(F("GAME START"), onStart);
    addMenuItem(F("SETTINGS"), onSettings);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuItemPos((state == STATE_SETTINGS) ? 1 : 0);
    setMenuCoords(22, 46, 83, 18, false, true);
    counter = 0;
    state = STATE_TOP;
    isInvalid = true;
    dprintln(F("Title screen"));
}

static void onStart(void)
{
    state = STATE_STARTED;
    dprintln(F("Game start"));
}

static void onSettings(void)
{
    playSoundClick();
    clearMenuItems();
    addMenuItem(F("SPEED"), onMenuSpeed);
    addMenuItem(F("ACCELERATION"), onMenuAccel);
    addMenuItem(F("DENSITY"), onMenuDensity);
    addMenuItem(F("THICKNESS"), onMenuThickness);
    addMenuItem(F("SIMPLE MODE"), onMenuSimpleMode);
    addMenuItem(F("EXIT"), onTop);
    setMenuItemPos(0);
    setMenuCoords(10, 8, 112, 35, false, false);
    state = STATE_SETTINGS;
    isInvalid = true;
    isSettingChenged = true;
    dprintln(F("Show settings"));
}

static void onCredit(void)
{
    playSoundClick();
    state = STATE_CREDIT;
    isInvalid = true;
    dprintln(F("Show credit"));
}

static void onMenuSpeed(void)
{
    playSoundClick();
    record.speed = circulateOne(record.speed, 1, SPEED_MAX);
    isSettingChenged = true;
}

static void onMenuAccel(void)
{
    playSoundClick();
    record.acceleration = circulate(record.acceleration, 1, ACCEL_MAX);
    isSettingChenged = true;
}

static void onMenuDensity(void)
{
    playSoundClick();
    record.density = circulateOne(record.density, 1, DENSITY_MAX);
    isSettingChenged = true;
}

static void onMenuThickness(void)
{
    playSoundClick();
    record.thickness = circulateOne(record.thickness, 1, THICKNESS_MAX);
    isSettingChenged = true;
}

static void onMenuSimpleMode(void)
{
    playSoundClick();
    record.simpleMode = !record.simpleMode;
    isSettingChenged = true;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawTop(void)
{
    if (isInvalid) {
        //arduboy.drawBitmap(0, 0, imgTitle, IMG_TITLE_W, IMG_TITLE_H, WHITE);
        arduboy.printEx(16, 8, F(APP_TITLE));
        arduboy.printEx(16, 14, F("TITLE SCREEN"));
#ifdef DEBUG
        arduboy.printEx(16, 20, F("DEBUG"));
#endif
    }
    (record.simpleMode) ? drawSimpleModeInstruction() : drawMenuItems(isInvalid);
}

static void drawSettings(void)
{
    if (isInvalid) {
        arduboy.printEx(34, 0, F("[SETTINGS]"));
        arduboy.drawFastHLine(0, 44, 128, WHITE);
        arduboy.printEx(10, 48, F("PLAY COUNT "));
        arduboy.print(record.playCount);
        arduboy.printEx(10, 54, F("PLAY TIME"));
        drawTime(76, 54, record.playFrames);
    }
    drawMenuItems(isInvalid);
    if (isSettingChenged) {
        for (int i = 0; i < 5; i++) {
            int16_t dx = 100 - (getMenuItemPos() == i) * 4, dy = i * 6 + 8;
            const char *p;
            switch (i) {
            case 0:
                arduboy.printEx(dx, dy, record.speed);
                break;
            case 1:
                p = accelText + record.acceleration * 5;
                arduboy.printEx(dx, dy, (const __FlashStringHelper *)p);
                break;
            case 2:
                arduboy.printEx(dx, dy, record.density);
                break;
            case 3:
                arduboy.printEx(dx, dy, record.thickness);
                break;
            case 4:
                arduboy.printEx(dx, dy, record.simpleMode ? F("ON ") : F("OFF"));
                break;
            }
        }
        isSettingChenged = false;
    }
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
