#include "common.h"

/*  Defines  */

#define INST_PAGE_MAX 4

enum STATE_T {
    STATE_INIT = 0,
    STATE_TITLE,
    STATE_LEVEL,
    STATE_SETTINGS,
    STATE_INST,
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
static void onInstruction(void);
static void onCredit(void);
static void handleInst(void);
static void setInstPiecePair(void);
static void setInstWinLine(void);
static void handleAnyButton(void);

static void drawTitleImage(void);
static void drawRecord(void);
static void drawSettingOnOff(void);
static void drawInst(void);
static void drawCredit(void);
static void drawText(const char *p, int lines);
static void drawOnOff(int8_t x, int8_t y, bool isOn);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[384] = { // 128x24
    0x00, 0x80, 0xE0, 0xF0, 0xF8, 0x3C, 0x0E, 0x06, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03,
    0x03, 0x06, 0x1E, 0xFC, 0xF8, 0xF0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xC0, 0xF0, 0x7C, 0x1F, 0x3F, 0xFE, 0xF8, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, 0x03, 0x83, 0xFE, 0xFE, 0x7C, 0x00,
    0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0xFF, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x00, 0x80, 0xE0, 0xF0, 0xF8, 0x3C, 0x0E, 0x06, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x03, 0x03, 0x06, 0x1E, 0xFC, 0xF8, 0xF0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x1F, 0x7F, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0xE0, 0xFF, 0xFF, 0x3F, 0x0F, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xE0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x80,
    0xE0, 0xF8, 0x3E, 0x0F, 0x0D, 0x0C, 0x0C, 0x0C, 0x0D, 0x0F, 0x1F, 0xFF, 0xFC, 0xE0, 0x80, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x02, 0x06, 0x1E, 0x3F, 0x7F, 0xF3, 0xE1, 0xC0, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1F, 0x7F, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0xE0, 0xFF, 0xFF, 0x3F, 0x0F, 0x00, 0x00, 0x00, 0x03, 0x7F, 0x0F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x06, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0E, 0x1E,
    0x1F, 0x3B, 0x73, 0xE1, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x07, 0x06,
    0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x04, 0x06, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0F,
    0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0F, 0x0F, 0x0E,
    0x08, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x07, 0x0F, 0x0E,
    0x0C, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x06, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0E,
    0x06, 0x07, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0F, 0x0F, 0x06, 0x00
};

PROGMEM static const uint8_t imgLargePiece[2][52] = { // 13x32 x2
    {
        0xF8, 0x5C, 0xAC, 0x5E, 0xBE, 0x7F, 0xBF, 0x7F, 0xFE, 0xFE, 0xFC, 0xFC, 0xF8, 0xFF, 0x55, 0xAA,
        0x55, 0xAA, 0x55, 0xAA, 0x77, 0xBB, 0xFF, 0xEE, 0xFF, 0xFF, 0xEF, 0x55, 0x8A, 0x55, 0xAA, 0x15,
        0xAA, 0x57, 0xBB, 0xDF, 0xBE, 0xDF, 0xEF, 0x1F, 0x35, 0x2A, 0x55, 0x6A, 0xD5, 0xAA, 0xF7, 0x7B,
        0x7F, 0x3E, 0x3F, 0x1F
    },{
        0xF8, 0x0C, 0x1C, 0x2A, 0x06, 0xC2, 0x46, 0xA2, 0x26, 0xEE, 0x7C, 0xBC, 0xF8, 0xFF, 0x00, 0x00,
        0x20, 0x00, 0x88, 0x00, 0xAA, 0x00, 0xAA, 0x55, 0xAA, 0xFF, 0xEF, 0x10, 0x00, 0x40, 0x00, 0x88,
        0x40, 0xAA, 0x40, 0xAA, 0x4D, 0xBA, 0xEF, 0x1F, 0x28, 0x20, 0x40, 0x40, 0x68, 0x40, 0x6A, 0x40,
        0x6A, 0x35, 0x2A, 0x1F
    }
};

PROGMEM static const char instText1[] = \
        "\"QUARTO!\" IS MADE OF\0A 4X4 SQUARE BOARD\0AND OF 16 DIFFERENT\0" \
        "PIECES, EACH OF WHICH\0HAS 4 ATTRIBUTES.\0\e";

PROGMEM static const char instText2[] = \
        "THE AIM IS TO LINE\0UP PIECES WITH\0THE SAME ATTRIBUTE.\0" \
        "THE ALIGNMENT CAN BE\0HORIZONTAL, VERTICAL\0OR DIAGONAL.\0\e";

PROGMEM static const char instText3[] = \
        "PLAYERS TAKE TURNS\0CHOOSING A PIECE\0WHICH THE OTHER\0" \
        "PLAYER MUST THEN\0PLACE ON THE BOARD.\0\e";

PROGMEM static const char instText4[] = \
        "A VARIANT RULE:\0A PLAYER ALSO CAN WIN\0BY PLACING 4 MATCHING\0" \
        "PIECES IN 2X2 SQUARE.\0\0THIS IS CONFIGURABLE.\0CURRENT SETTING:    \0\e";

PROGMEM static const char * const instList[] = { instText1, instText2, instText3, instText4 };

PROGMEM static const char instAttrLabels[] = \
        "   PLANE\0HOLLOW  \0    TALL\0SHORT   \0  SQUARE\0CIRCULAR\0   SOLID\0MESHED  ";

PROGMEM static const char creditText[] = \
        "- " APP_TITLE " -\0\0\0" APP_RELEASED "\0PROGREMMED BY OBONO\0\0" \
        "THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.\0\0\0" \
        "ORIGINAL CONCEPT BY\0BLAISE MULLER, FRANCE\0\e";

static STATE_T  state = STATE_INIT;
static uint8_t  counter, instPage, instAttr, instPieces[BOARD_SIZE];
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
    case STATE_LEVEL:
    case STATE_SETTINGS:
        handleMenu();
        if (state == STATE_LEVEL && arduboy.buttonDown(A_BUTTON)) onBack();
        if (state == STATE_SETTINGS) {
            if (arduboy.buttonDown(UP_BUTTON | DOWN_BUTTON)) isSettingsChanged = true;
            if (arduboy.buttonDown(A_BUTTON)) onBack();
        }
        if (state == STATE_STARTED) {
            ret = MODE_GAME;
        }
        break;
    case STATE_INST:
        handleInst();
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
        case STATE_LEVEL:
            drawTitleImage();
            break;
        case STATE_SETTINGS:
            drawRecord();
            break;
        case STATE_INST:
            drawInst();
            break;
        case STATE_CREDIT:
            drawCredit();
            break;
        }
    }
    if (state == STATE_TITLE || state == STATE_LEVEL || state == STATE_SETTINGS) {
        drawMenuItems(isInvalid);
        if (isSettingsChanged) {
            drawSettingOnOff();
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
    addMenuItem(F("INSTRUCTION"), onInstruction);
    addMenuItem(F("CREDIT"), onCredit);
    setMenuCoords(22, 28, 83, 36, false, true);
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
    setMenuCoords(34, 28, 59, 36, false, false);
    setMenuItemPos(record.cpuLevel - 1);
    state = STATE_LEVEL;
    isInvalid = true;
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
    addMenuItem(F("APPROVE 2X2"), onSettingChange);
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
    if (getMenuItemPos() == 2) {
        invertScreen(record.settings & SETTING_BIT_SCREEN_INV);
    }
    dprint(F("Setting changed: "));
    dprintln(getMenuItemPos());
}

static void onInstruction(void)
{
    playSoundClick();
    counter = 0;
    instPage = 0;
    instAttr = 0;
    setInstPiecePair();
    state = STATE_INST;
    isInvalid = true;
    dprintln(F("Show instruction"));
}

static void onCredit(void)
{
    playSoundClick();
    state = STATE_CREDIT;
    isInvalid = true;
    dprintln(F("Show credit"));
}

static void handleInst(void)
{
    int8_t vp = (arduboy.buttonDown(DOWN_BUTTON | RIGHT_BUTTON | B_BUTTON)
                    && instPage < INST_PAGE_MAX - 1)
              - (arduboy.buttonDown(UP_BUTTON | LEFT_BUTTON) && instPage > 0);
    if (vp) {
        playSoundTick();
        instPage += vp;
        isInvalid = true;
        dprint(F("instPage = "));
        dprintln(instPage);
    } else if (arduboy.buttonDown(A_BUTTON)
            || arduboy.buttonDown(B_BUTTON) && instPage == INST_PAGE_MAX - 1) {
        playSoundClick();
        state = STATE_TITLE;
        isInvalid = true;
    }

    if (vp || ++counter >= FPS * 3) {
        if (instPage == 0) setInstPiecePair();
        if (instPage == 1) setInstWinLine();
        counter = 0;
    }
}

static void setInstPiecePair(void)
{
    instAttr = (instAttr + 1) % PIECE_ATTRS;
    uint8_t attrBit = 1 << instAttr;
    instPieces[0] = random(16) & ~attrBit;
    instPieces[1] = instPieces[0] | attrBit;
    isInvalid = true;
}

static void setInstWinLine(void)
{
    instAttr = (instAttr + 1) % PIECE_ATTRS;
    uint8_t attrBit = 1 << instAttr;
    bool attrActive = random(2);
    uint16_t usedPieces = 0;
    for (uint8_t i = 0; i < BOARD_SIZE; i++) {
        uint8_t piece;
        do {
            piece = random(16);
            if (attrActive) {
                piece |= attrBit;
            } else {
                piece &= ~attrBit;
            }
        } while (usedPieces & 1 << piece);
        instPieces[i] = piece;
        usedPieces |= 1 << piece;
    }
    isInvalid = true;
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
    arduboy.printEx(0, 0, F("DEBUG"));
#endif
    arduboy.drawBitmap(0, 2, imgTitle, 128, 24, WHITE);
    arduboy.drawBitmap(4, 24, imgLargePiece[0], 13, 32, WHITE);
    arduboy.drawBitmap(111, 24, imgLargePiece[1], 13, 32, WHITE);
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

static void drawSettingOnOff(void)
{
    int8_t pos = getMenuItemPos();
    for (int i = 0; i < 4; i++) {
        drawOnOff(106 - (i == pos) * 4, i * 6 + 12, record.settings & 1 << i);
    }
}

static void drawInst(void)
{
    arduboy.printEx(13, 2, F("[INSTRUCTION  /4]"));
    drawNumber(91, 2, instPage + 1);
    drawText((const char *) pgm_read_ptr(instList + instPage), 12);

    /*  Attribute samples  */
    if (instPage == 0) {
        const char *p = instAttrLabels + instAttr * 18;
        arduboy.printEx(0, 58, (const __FlashStringHelper *) p);
        arduboy.printEx(80, 58, (const __FlashStringHelper *) (p + 9));
        drawPiece(48, 48, instPieces[0]);
        drawPiece(64, 48, instPieces[1]);
    }
    /*  Win-condition samples */
    if (instPage == 1) {
        for (uint8_t i = 0; i < BOARD_SIZE; i++) {
            drawPiece(i * 16 + (WIDTH - 16 * BOARD_SIZE) / 2, 48, instPieces[i]);
        }
    }
    /*  Variant rule on / off  */
    if (instPage == 3) {
        drawOnOff(106, 44, record.settings & SETTING_BIT_2x2_RULE);
    }
}

static void drawCredit(void)
{
    drawText(creditText, 3);
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

static void drawOnOff(int8_t x, int8_t y, bool isOn)
{
    arduboy.printEx(x, y, (isOn) ? F("ON ") : F("OFF"));
}
