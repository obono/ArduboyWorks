#include "common.h"

/*  Defines  */

enum {
    STATE_TITLE = 0,
    STATE_PUZZLE,
    STATE_GALLERY,
    STATE_CREDIT,
};

enum {
    MENU_CONTINUE = 0,
    MENU_PUZZLE,
    MENU_GALLERY,
    MENU_SOUND,
    MENU_HELP,
    MENU_CREDIT,
    MENU_RESET,
};

/*  Typedefs  */

typedef struct
{
    uint8_t id;
    bool (*func)(void);
    const __FlashStringHelper *label;
} ITEM_T;

/*  Local Functions  */

static void setupMenuItems(void);
static void addMenuItem(uint8_t id, const __FlashStringHelper *label, bool(*func)(void));

static bool handleButtons(void);
static void handleWaiting(void);

static bool onContinue(void);
static bool onPuzzle(void);
static bool onGallery(void);
static bool onSound(void);
static bool onHelp(void);
static bool onCredit(void);
static bool onReset(void);

static void drawMenuItems(void);
static void drawMenuOnOff(bool on);
static void drawCredit(void);

/*  Local Variables  */

PROGMEM static const uint8_t imgTitle[512] = { // 128x32
    0x00, 0x00, 0x80, 0xE0, 0xFE, 0x7E, 0x3E, 0xFE, 0xEC, 0xE0, 0xE0, 0x20, 0x30, 0x38, 0xB8, 0xB0,
    0x20, 0xFC, 0xFC, 0xF8, 0xF0, 0xF0, 0x10, 0x10, 0x10, 0xF8, 0xF8, 0xF8, 0xF0, 0xF0, 0x00, 0x00,
    0x10, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xFE, 0xFE, 0xFE, 0xFE, 0xB2, 0xFC, 0xDC, 0xD8, 0x90,
    0x90, 0x90, 0x90, 0xFE, 0xFE, 0xFE, 0xFE, 0x96, 0x90, 0xD0, 0xFC, 0xFC, 0xD8, 0x90, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0xFE, 0xFC, 0x04, 0x00,
    0x00, 0xFE, 0xFE, 0xFE, 0xFC, 0x04, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0xFE, 0xFE, 0x84, 0xC0, 0xC0, 0x80, 0xF8, 0xF8, 0xF8,
    0xF0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x18, 0x1E, 0x1E, 0x1C, 0x18, 0x10, 0x00,
    0x02, 0x02, 0x03, 0x03, 0x82, 0xC2, 0xFA, 0xFF, 0xFF, 0x3F, 0x1F, 0x32, 0xF2, 0xF3, 0xE3, 0xE3,
    0x83, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x20, 0x20, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x00, 0x00,
    0x04, 0x1C, 0x1C, 0x5C, 0x4C, 0x4C, 0x4C, 0x7F, 0x7F, 0x5F, 0x5F, 0x46, 0x42, 0x42, 0x46, 0x46,
    0x44, 0x44, 0x44, 0x5F, 0x5F, 0x5F, 0xFF, 0xF4, 0xF4, 0xE4, 0xC6, 0x87, 0x07, 0x06, 0x04, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02,
    0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x04, 0x00, 0x00,
    0x02, 0x02, 0x02, 0x82, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0xC3, 0x83, 0x03, 0xFF, 0xFF, 0xFF,
    0xFF, 0x18, 0xF8, 0xE8, 0x08, 0x08, 0x08, 0xE8, 0xFC, 0xFC, 0xFC, 0x78, 0x30, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x06, 0x07, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x42, 0x42, 0x42, 0x43, 0x43, 0x43,
    0x42, 0x42, 0x42, 0x42, 0x42, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0xD2, 0xD2, 0x12, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0x52, 0xD2, 0xD2, 0xD2,
    0xD2, 0x92, 0x12, 0x12, 0x12, 0x92, 0xFF, 0xFF, 0xFF, 0xBF, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02,
    0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x02, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x40, 0x70, 0x3C, 0x1F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x07, 0x07, 0xE7, 0xFF, 0xFF, 0x7F,
    0x1F, 0x00, 0x03, 0x9F, 0xFF, 0xFC, 0xFE, 0xFF, 0xFF, 0x8F, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x1F, 0x3F, 0x3F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x1C, 0x1E, 0x1F, 0x1F, 0x0F, 0x00, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x38, 0x38, 0x39, 0x3B,
    0x3B, 0x3B, 0x38, 0x3C, 0x3F, 0x3F, 0x3F, 0x18, 0x01, 0x0F, 0x1F, 0x1F, 0x1E, 0x1E, 0x00, 0x00,
    0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x08, 0x08, 0x08, 0x0F, 0x0F, 0x0F, 0x0F, 0x08, 0x08,
    0x08, 0x0F, 0x0F, 0x0F, 0x0F, 0x08, 0x08, 0x08, 0x3F, 0x3F, 0x3F, 0x1F, 0x1F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x20, 0x38, 0x1E, 0x2F, 0x27, 0x31, 0x18,
    0x1C, 0x0E, 0x0F, 0x07, 0x03, 0x01, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x1E, 0x06, 0x06, 0x00,
};

PROGMEM static const char creditText[] = "- " APP_TITLE " -\0\0" APP_RELEASED \
        "\0PROGREMMED BY OBONO\0\0THIS PROGRAM IS\0" "RELEASED UNDER\0" "THE MIT LICENSE.";

static uint8_t  state = STATE_TITLE;
static bool     toDraw;
static bool     toDrawFrame;
static int8_t   menuCount;
static ITEM_T   menuItemAry[5];
static int8_t   menuPos;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initMenu(void)
{
    setupMenuItems();
    menuPos = 0;
    toDraw = true;
    toDrawFrame = true;
}

bool updateMenu(void)
{
    bool ret = false;
    if (state == STATE_CREDIT) {
        handleWaiting();
    } else {
        ret = handleButtons();
    }
    return ret;
}

void drawMenu(void)
{
    if (toDraw) {
        if (state == STATE_CREDIT) {
            drawCredit();
        } else {
            drawMenuItems();
        }
        toDraw = false;
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void setupMenuItems(void)
{
    menuCount = 0;
    if (state != STATE_TITLE) {
        addMenuItem(MENU_CONTINUE, F("CONTINUE"), onContinue);
    }
    if (state != STATE_PUZZLE) {
        addMenuItem(MENU_PUZZLE, F("PLAY PUZZLE"), onPuzzle);
    }
    if (state != STATE_GALLERY && clearCount > 0) {
        addMenuItem(MENU_GALLERY, F("GALLERY"), onGallery);
    }
    addMenuItem(MENU_SOUND, F("SOUND"), onSound);
    addMenuItem(MENU_HELP, F("SHOW HELP"), onHelp);
    if (state == STATE_TITLE) {
        addMenuItem(MENU_CREDIT, F("CREDIT"), onCredit);
    } else if (state == STATE_PUZZLE) {
        addMenuItem(MENU_RESET, F("RESET"), onReset);
    }
}

static void addMenuItem(uint8_t id, const __FlashStringHelper *label, bool(*func)(void))
{
    ITEM_T *pItem = &menuItemAry[menuCount];
    pItem->id = id;
    pItem->label = label;
    pItem->func = func;
    menuCount++;
}

static bool handleButtons()
{
    bool ret = false;
    if (arduboy.buttonDown(UP_BUTTON) && menuPos > 0) {
        menuPos--;
        playSoundTick();
        toDraw = true;
    }
    if (arduboy.buttonDown(DOWN_BUTTON) && menuPos < menuCount - 1) {
        menuPos++;
        playSoundTick();
        toDraw = true;
    }
    if (arduboy.buttonDown(B_BUTTON)) {
        ret = menuItemAry[menuPos].func();
    } else if (arduboy.buttonDown(A_BUTTON) && state != STATE_TITLE) {
        ret = onContinue();
    }
    return ret;
}

static void handleWaiting(void)
{
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        playSoundClick();
        state = STATE_TITLE;
        toDraw = true;
        toDrawFrame = true;
    }
}

static bool onContinue(void)
{
    playSoundTick();
    return true;
}

static bool onPuzzle(void)
{
    playSoundClick();
    state = STATE_PUZZLE;
    return true;
}

static bool onGallery(void)
{
    playSoundClick();
    state = STATE_GALLERY;
    return true;
}

static bool onSound(void)
{
    playSoundClick();
    isSoundEnable = !isSoundEnable;
    if (isSoundEnable) {
        arduboy.audio.on();
    } else {
        arduboy.audio.off();
    }
    toDraw = true;
    return false;
}

static bool onHelp(void)
{
    playSoundClick();
    isHelpVisible = !isHelpVisible;
    toDraw = true;
    return false;
}

static bool onCredit(void)
{
    playSoundClick();
    state = STATE_CREDIT;
    toDraw = true;
    return false;
}

static bool onReset(void)
{
    playSoundClick();
    resetPieces();
    state = STATE_PUZZLE;
    return true;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawMenuItems(void)
{
    /*  Clear  */
    int8_t  menuTop = (state == STATE_TITLE) ? 34 : 30 - menuCount * 3;
    uint8_t menuHeight = menuCount * 6;
    if (toDrawFrame) {
        if (state == STATE_TITLE) {
            arduboy.clear();
            arduboy.drawBitmap(0, 0, imgTitle, 128, 32, WHITE);
        } else {
            arduboy.drawRect(18, menuTop - 2, 91, menuHeight + 3, WHITE);
        }
        toDrawFrame = false;
    }
    arduboy.fillRect(19, menuTop - 1, 89, menuHeight + 1, BLACK);

    /*  Menu items  */
    ITEM_T *pItem = menuItemAry;
    for (int i = 0; i < menuCount; i++, pItem++) {
        arduboy.printEx(30 - (i == menuPos) * 4, i * 6 + menuTop, pItem->label);
        if (pItem->id == MENU_SOUND) drawMenuOnOff(isSoundEnable);
        if (pItem->id == MENU_HELP)  drawMenuOnOff(isHelpVisible);
    }
    arduboy.fillRect2(20, menuPos * 6 + menuTop, 5, 5, WHITE);
}

static void drawMenuOnOff(bool on)
{
    arduboy.print((on) ? F(" ON") : F(" OFF"));
}

static void drawCredit(void)
{
    arduboy.clear();
    char buf[20];
    const char *p = creditText;
    for (int i = 0; i < 8; i++) {
        strcpy_P(buf, p);
        uint8_t len = strnlen(buf, sizeof(buf));
        p += arduboy.printEx(64 - len * 3, i * 6 + 8, buf) + 1;
    }
}