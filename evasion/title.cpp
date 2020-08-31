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
    STATE_NOTICE,
    STATE_STARTED,
};

#define IMG_TITLE_W     88
#define IMG_TITLE_H     24
#define IMG_GUYPART_H   16
#define IMG_BUTTON_W    7
#define IMG_BUTTON_H    7

/*  Typedefs  */

typedef struct {
    const uint8_t *bitmap;
    uint8_t offsetX, width;
} GUYPART_T;

/*  Local Functions  */

static void handleInit(void);
static void handleTop(void);
static void handleSettings(void);
static void handleNotice(void);
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

static void drawInit(void);
static void drawTop(void);
static void drawSettings(void);
static void drawCredit(void);
static void drawNotice(void);

static void drawGuy(int16_t x);
static void drawText(const char *p, int lines);

/*  Local Functions (macros)  */

#define callHandlerFunc(n)  ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()
#define callDrawerFunc(n)   ((void (*)(void)) pgm_read_ptr(drawerFuncTable + n))()
#define getGameSettings()   (*((uint16_t *)&record) & 0xFFF) // trick!

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[264] = { // 88x24
    0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xC0, 0xC0, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0xFF, 0xFF, 0xFF, 0xC1, 0x61, 0x61,
    0x61, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0x20, 0xE0, 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0xE0, 0xE0, 0x60, 0x00, 0x00, 0x80, 0xE0, 0xF0, 0x70, 0x30, 0x10, 0x10, 0xF0, 0xF0, 0xE0,
    0x80, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0x70, 0x70, 0x30, 0x30, 0x30, 0x70, 0x60, 0x00, 0x00,
    0xC6, 0xCE, 0xCE, 0x08, 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF0, 0x70, 0x30, 0x30, 0xF0, 0xE0, 0x80,
    0x00, 0x00, 0x00, 0xE0, 0xE0, 0xE0, 0x80, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7F, 0x7F, 0x7F, 0x60, 0x60, 0x60, 0x60, 0x20, 0x30, 0x30, 0x10, 0x10, 0x00, 0x00,
    0x00, 0x01, 0x1F, 0x3F, 0x7C, 0x70, 0x3C, 0x3E, 0x0F, 0x07, 0x01, 0x00, 0x00, 0x3E, 0x7F, 0x7F,
    0x61, 0x70, 0x38, 0x1C, 0x0E, 0x0F, 0x7F, 0xFF, 0xE0, 0x80, 0x00, 0x00, 0xC0, 0xC1, 0x83, 0x87,
    0xCE, 0xDC, 0xF8, 0x70, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0x00, 0x00, 0x1F, 0x3F, 0x7F,
    0x61, 0x40, 0x60, 0x60, 0x60, 0x30, 0x1F, 0x1F, 0x0E, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x1F, 0x03,
    0x01, 0x00, 0x07, 0x3F, 0x7F, 0x78, 0x40, 0x40
};


PROGMEM static const uint8_t imgGuyPart0[] = { // 36x16  +2
    0x00, 0x00, 0x60, 0xA0, 0x2C, 0x14, 0x08, 0x0F, 0x01, 0x02, 0x1C, 0x08, 0x08, 0x34, 0x4C, 0x20,
    0x10, 0xF0, 0xC0, 0x60, 0x30, 0xF8, 0xF8, 0xF8, 0xFC, 0x7C, 0xBC, 0x78, 0xB0, 0x70, 0xB0, 0xD0,
    0xE0, 0xE0, 0xC0, 0x00, 0x02, 0x46, 0xA9, 0x30, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00,
    0x00, 0x00, 0xC0, 0xE0, 0xE0, 0x00, 0x00, 0xA4, 0xF7, 0xFB, 0x7F, 0xFF, 0xFF, 0xFE, 0xFE, 0xFE,
    0xFC, 0xF8, 0xF9, 0xF3, 0xE7, 0xE7, 0xC3, 0x80
};

PROGMEM static const uint8_t imgGuyPart1[] = { // 46x16  +0
    0xE0, 0xF0, 0xF0, 0xF0, 0xE0, 0xE1, 0xC1, 0x02, 0x87, 0x07, 0x07, 0x0F, 0x1F, 0x19, 0x1F, 0x1F,
    0x1F, 0x0F, 0x07, 0x02, 0x03, 0x02, 0x85, 0xCA, 0xF5, 0xEE, 0xF7, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF,
    0x5F, 0xAF, 0x5F, 0xBF, 0xDF, 0xFF, 0x7F, 0x7E, 0x3C, 0x38, 0xB0, 0x80, 0x00, 0x00, 0x01, 0x01,
    0x03, 0x03, 0x03, 0x05, 0x06, 0x0F, 0x0F, 0x1F, 0x1F, 0x3E, 0x3E, 0x7E, 0x7E, 0xFE, 0xFC, 0xFC,
    0xF8, 0xF8, 0xFE, 0xFF, 0xFF, 0x7F, 0x3F, 0x9F, 0xCF, 0xC7, 0xE3, 0xE1, 0xE5, 0xF2, 0x75, 0xF2,
    0xF9, 0xF9, 0x7C, 0xFC, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x80
};

PROGMEM static const uint8_t imgGuyPart2[] = { // 47x16  +16
    0x00, 0x00, 0x01, 0x81, 0xE1, 0xF1, 0xF8, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
    0x3F, 0x9E, 0x4D, 0x82, 0x45, 0x8B, 0x9F, 0x3F, 0x5F, 0xBF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0xFC, 0xF8, 0xF0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70,
    0x7C, 0xFF, 0xFF, 0x7F, 0x7F, 0x7F, 0x3F, 0x3F, 0x1F, 0x1F, 0x1F, 0x0F, 0x0D, 0x06, 0x06, 0x07,
    0x06, 0x03, 0x03, 0x07, 0x07, 0x07, 0x07, 0x0E, 0x0C, 0x09, 0x02, 0x05, 0x0B, 0x05, 0x0B, 0x17,
    0x2F, 0x5F, 0x3F, 0x7F, 0xFE, 0x7E, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8, 0xF0, 0xC0, 0x80
};

PROGMEM static const uint8_t imgGuyPart3[] = { // 15x16  +56
    0x01, 0x02, 0x05, 0x03, 0x07, 0x0F, 0x17, 0x2F, 0xD0, 0xFE, 0xFE, 0xFE, 0xFC, 0xC0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x3D, 0x3F, 0x3F, 0x1F, 0x0F
};

PROGMEM static const GUYPART_T guyPartTable[] = {
    { imgGuyPart0, 2, 36 }, { imgGuyPart1, 0, 46 }, { imgGuyPart2, 16, 47 }, { imgGuyPart3, 56, 15 }
};

PROGMEM static const uint8_t imgButton[2][7] = { // 7x7 x2
    { 0x3E, 0x47, 0x6B, 0x6D, 0x6D, 0x41, 0x3E }, { 0x3E, 0x41, 0x55, 0x55, 0x51, 0x65, 0x3E }
};

PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGREMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\e";
PROGMEM static const char accelText[] = "OFF \0SLOW\0FAST";
PROGMEM static const char noticeText[] = \
        "[SIMPLE MODE]\0\0\0IN THIS MODE,\0YOU CAN ACCESS\0THE SETTINGS BY\0" \
        "HOLDING D-PAD DOWN.\0\0  OK    CANCEL\0\e";

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleInit, handleTop, handleSettings, handleAnyButton, handleNotice
};

PROGMEM static void(*const drawerFuncTable[])(void) = {
    drawInit, drawTop, drawSettings, drawCredit, drawNotice
};

static STATE_T  state = STATE_INIT;
static uint16_t lastSettings;
static bool     lastSimpleMode, isSettingChenged;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    if (state == STATE_INIT) {
        counter = 0;
    } else if (record.simpleMode && state == STATE_STARTED) {
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

static void handleInit(void)
{
    if (++counter >= FPS) onTop();
    isInvalid = true;
}

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
    if (arduboy.buttonDown(UP_BUTTON | DOWN_BUTTON)) isSettingChenged = true;
}

static void handleNotice(void)
{
    if (arduboy.buttonDown(A_BUTTON)) {
        onTop();
    } else if (arduboy.buttonDown(B_BUTTON)) {
        record.simpleMode = false;
        onSettings();
    }
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
    if (state == STATE_SETTINGS || state == STATE_NOTICE) {
        playSoundClick();
        if (state == STATE_SETTINGS && record.simpleMode && !lastSimpleMode) {
            state = STATE_NOTICE;
            isInvalid = true;
            return;
        }
        if (getGameSettings() != lastSettings) {
            record.hiscore = 0;
            isRecordDirty = true;
        }
        if (record.simpleMode != lastSimpleMode) isRecordDirty = true;
        writeRecord();
    }
    clearMenuItems();
    addMenuItem(F("GAME START"), onStart);
    addMenuItem(F("SETTINGS"), onSettings);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuItemPos((state == STATE_SETTINGS) ? 1 : 0);
    setMenuCoords(56, 2, 72, 17, false, true);
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
    setMenuItemPos((state == STATE_NOTICE) ? 4 : 0);
    setMenuCoords(10, 8, 112, 35, false, false);
    if (state != STATE_NOTICE) {
        lastSettings = getGameSettings();
        lastSimpleMode = record.simpleMode;
    }
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

static void drawInit(void)
{
    int16_t guyX = 0;
    if (counter < FPS / 2) {
        int16_t tmp = FPS / 2 - counter;
        guyX = tmp * tmp / 7;
    }
    drawGuy(guyX);
    if (counter >= FPS / 2) {
        int16_t tmp = FPS - counter;
        int16_t x = WIDTH - IMG_TITLE_W - tmp * tmp / 7;
        drawBitmapBordered(x, 16, imgTitle, IMG_TITLE_W, IMG_TITLE_H);
    }
}

static void drawTop(void)
{
    if (isInvalid) {
        drawGuy(0);
        drawBitmapBordered(WIDTH - IMG_TITLE_W, 16, imgTitle, IMG_TITLE_W, IMG_TITLE_H);
        if (record.hiscore > 0) {
            arduboy.printEx(0, 50, F("HI:"));
            arduboy.print(record.hiscore / 6);
        }
#ifdef DEBUG
        arduboy.printEx(98, 40, F("DEBUG"));
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
    if (isInvalid) drawText(creditText, 11);
}

static void drawNotice(void)
{
    if (isInvalid) {
        arduboy.drawRect(2, 7, 123, 49, WHITE);
        drawText(noticeText, 11);
        for (int i = 0; i < 2; i++) {
            arduboy.drawBitmap(21 + i * 36, 46, imgButton[i], IMG_BUTTON_W, IMG_BUTTON_H);
        }
    }
}

/*---------------------------------------------------------------------------*/

static void drawGuy(int16_t x)
{
    GUYPART_T guyPart;
    for (int i = 0; i < 4; i++) {
        memcpy_P(&guyPart, guyPartTable + i, sizeof(GUYPART_T));
        arduboy.drawBitmap(x + guyPart.offsetX, IMG_GUYPART_H * i, guyPart.bitmap,
                guyPart.width, IMG_GUYPART_H);
    }
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
