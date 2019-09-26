#include "common.h"

/*  Defines  */

enum STATE_T {
    STATE_INIT = 0,
    STATE_TITLE,
    STATE_SETTINGS,
    STATE_CREDIT,
    STATE_STARTED,
};

/*  Local Functions  */

static void initTitleMenu(bool isFromSettings);
static void onVsCpu(void);
static void on2Players(void);
static void onLevel(void);
static void onBack(void);
static void onSettings(void);
static void onSettingChange(void);
static void onCredit(void);
static void handleAnyButton(void);

static void drawTitleImage(void);
static void drawRecord(void);
static void drawCredit(void);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[] = {
};

PROGMEM static const char creditText[] = "- " APP_TITLE " -\0\0" APP_RELEASED \
        "\0PROGREMMED BY OBONO\0\0THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.";

static STATE_T  state = STATE_INIT;
static bool     isSettingsChanged;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    if (state == STATE_INIT) {
        readRecord();
        invertScreen(record.settings & SETTING_BIT_SCREEN_INV);
    }
    initTitleMenu(false);
}

MODE_T updateTitle(void)
{
    MODE_T ret = MODE_TITLE;
    switch (state) {
    case STATE_TITLE:
    case STATE_SETTINGS:
        handleMenu();
        if (state == STATE_STARTED) {
            ret = MODE_GAME;
        }
        break;
    case STATE_CREDIT:
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
        case STATE_SETTINGS:
            drawRecord();
            break;
        case STATE_CREDIT:
            drawCredit();
            break;
        }
    }
    if (state == STATE_TITLE || state == STATE_SETTINGS) {
        drawMenuItems(isInvalid);
        if (state == STATE_SETTINGS &&
                (arduboy.buttonDown(UP_BUTTON | DOWN_BUTTON) || isSettingsChanged)) {
            int8_t pos = getMenuItemPos();
            for (int i = 0; i < 3; i++) {
                bool on = (record.settings & 1 << i);
                arduboy.printEx(106 - (i == pos) * 4, i * 6 + 12, (on) ? F("ON ") : F("OFF"));
            }
            isSettingsChanged = false;
        }
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void initTitleMenu(bool isFromSettings)
{
    clearMenuItems();
    addMenuItem(F("YOU FIRST"), onVsCpu);
    addMenuItem(F("CPU FIRST"), onVsCpu);
    addMenuItem(F("2 PLAYERS"), on2Players);
    setMenuItemPos((isFromSettings) ? 3 : record.gameMode);
    addMenuItem(F("SETTINGS"), onSettings);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuCoords(57, 28, 71, 30, false, true);
    if (isFromSettings) isInvalid = true;
    state = STATE_TITLE;
    isInvalid = true;
    dprintln(F("Menu: title"));
}

static void onVsCpu(void)
{
    record.gameMode = (getMenuItemPos() == 0) ? GAME_MODE_YOU_FIRST : GAME_MODE_CPU_FIRST;
    playSoundClick();

    int maxLevel = max(record.maxLevel, 3);
    clearMenuItems();
    addMenuItem(F("LEVEL 1"), onLevel);
    addMenuItem(F("LEVEL 2"), onLevel);
    addMenuItem(F("LEVEL 3"), onLevel);
    if (maxLevel >= 4) addMenuItem(F("LEVEL 4"), onLevel);
    if (maxLevel >= 5) addMenuItem(F("LEVEL 5"), onLevel);
    addMenuItem(F("BACK"), onBack);
    setMenuCoords(57, 28, 71, 36, false, false);
    setMenuItemPos(record.cpuLevel - 1);
    dprintln(F("Menu: level"));
}

static void on2Players(void)
{
    record.gameMode = GAME_MODE_2PLAYERS;
    state = STATE_STARTED;
    dprintln(F("Start 2 players game"));
}
static void onLevel(void)
{
    record.cpuLevel = getMenuItemPos() + 1;
    state = STATE_STARTED;
    dprint(F("Start vs CPU: level="));
    dprint(record.cpuLevel);
    dprintln((record.gameMode == GAME_MODE_YOU_FIRST) ? F(" You first") : F(" CPU first"));
}

static void onBack(void)
{
    playSoundClick();
    initTitleMenu(state == STATE_SETTINGS);
}

static void onSettings(void)
{
    playSoundClick();
    clearMenuItems();
    addMenuItem(F("THINKING LED"), onSettingChange);
    addMenuItem(F("INVERT SCREEN"), onSettingChange);
    addMenuItem(F("HINT (VS CPU)"), onSettingChange);
    addMenuItem(F("EXIT"), onBack);
    setMenuCoords(4, 12, 120, 30, false, false);
    setMenuItemPos(0);
    state = STATE_SETTINGS;
    isInvalid = true;
    isSettingsChanged = true;
    dprintln(F("Menu: settings"));
}

static void onSettingChange(void)
{
    playSoundTick();
    record.settings ^= 1 << getMenuItemPos();
    isSettingsChanged = true;
    if (getMenuItemPos() == 1) {
        invertScreen(record.settings & SETTING_BIT_SCREEN_INV);
    }
    dprint(F("Setting changed: "));
    dprintln(getMenuItemPos());
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
    arduboy.printEx(0, 6, F("TITLE SCREEN"));
    //arduboy.drawBitmap(0, 12, imgTitle, 16, 16, WHITE);
}

static void drawRecord(void)
{
    arduboy.printEx(34, 4, F("[SETTINGS]"));
    arduboy.drawFastHLine2(0, 44, 128, WHITE);
    arduboy.printEx(10, 48, F("PLAY COUNT "));
    arduboy.print(record.playCount);
    arduboy.printEx(10, 54, F("PLAY TIME"));
    drawTime(76, 54, record.playFrames);
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
