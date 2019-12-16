#include "common.h"

/*  Defines  */

#define MENU_COUNT_MAX  6

/*  Typedefs  */

typedef struct
{
    void (*func)(void);
    const __FlashStringHelper *label;
} ITEM_T;

/*  Local Variables  */

PROGMEM static const uint8_t imgSound[14] = {
    0x3E, 0x47, 0x6B, 0x6D, 0x6D, 0x41, 0x3E, 0x00, 0x1C, 0x1C, 0x00, 0x1C, 0x3E, 0x7F
};

PROGMEM static const uint8_t imgSoundOffOn[2][6] = {
    { 0x00, 0x00, 0x50, 0x20, 0x50, 0x00 },
    { 0x14, 0x08, 0x22, 0x1C, 0x41, 0x3E },
};

static int8_t   menuX, menuY, menuW, menuH;
static bool     isFramed, isControlSound;
static int8_t   menuItemCount;
static ITEM_T   menuItemAry[MENU_COUNT_MAX];
static int8_t   menuItemPos;
static bool     isInvalidMenu;

/*---------------------------------------------------------------------------*/

void clearMenuItems(void)
{
    menuItemCount = 0;
    dprintln(F("Clear menu items"));
}

void addMenuItem(const __FlashStringHelper *label, void (*func)(void))
{
    if (menuItemCount >= MENU_COUNT_MAX) return;
    ITEM_T *pItem = &menuItemAry[menuItemCount];
    pItem->label = label;
    pItem->func = func;
    menuItemCount++;
    dprint(F("Add menu items: "));
    dprintln(label);
}

int8_t getMenuItemPos(void)
{
    return menuItemPos;
}

int8_t getMenuItemCount(void)
{
    return menuItemCount;
}

void setMenuCoords(int8_t x, int8_t y, int8_t w, int8_t h, bool f, bool s)
{
    menuX = x;
    menuY = y;
    menuW = w;
    menuH = h;
    isFramed = f;
    isControlSound = s;
}

void setMenuItemPos(int8_t pos)
{
    menuItemPos = pos;
    isInvalidMenu = true;
    dprint(F("menuItemPos="));
    dprintln(menuItemPos);
}

void setConfirmMenu(int8_t y, void (*funcOk)(), void (*funcCancel)())
{
    playSoundClick();
    clearMenuItems();
    addMenuItem(F("OK"), funcOk);
    addMenuItem(F("CANCEL"), funcCancel);
    setMenuCoords(40, y, 47, 11, true, true);
    setMenuItemPos(1);
    isInvalidMenu = true;
}

void handleMenu(void)
{
    if (arduboy.buttonDown(UP_BUTTON) && menuItemPos > 0) {
        menuItemPos--;
        playSoundTick();
        isInvalidMenu = true;
        dprint(F("menuItemPos="));
        dprintln(menuItemPos);
    }
    if (arduboy.buttonDown(DOWN_BUTTON) && menuItemPos < menuItemCount - 1) {
        menuItemPos++;
        playSoundTick();
        isInvalidMenu = true;
        dprint(F("menuItemPos="));
        dprintln(menuItemPos);
    }
    if (isControlSound && arduboy.buttonDown(A_BUTTON)) {
        setSound(!arduboy.isAudioEnabled());
        playSoundClick();
        isInvalidMenu = true;
    }
    if (arduboy.buttonDown(B_BUTTON)) {
        menuItemAry[menuItemPos].func();
    }
}

void drawMenuItems(bool isForced)
{
    if (!isInvalidMenu && !isForced) return;
    arduboy.fillRect2(menuX - 1, menuY - 1, menuW + 2, menuH + 2, BLACK);
    if (isFramed) {
        arduboy.drawRect2(menuX - 2, menuY - 2, menuW + 4, menuH + 4, WHITE);
    }
    ITEM_T *pItem = menuItemAry;
    for (int i = 0; i < menuItemCount; i++, pItem++) {
        arduboy.printEx(menuX + 12 - (i == menuItemPos) * 4, menuY + i * 6, pItem->label);
    }
    arduboy.fillRect2(menuX, menuY + menuItemPos * 6, 5, 5, WHITE);
    if (isControlSound) drawSoundEnabled();
    isInvalidMenu = false;
}

void drawSoundEnabled(void)
{
    arduboy.fillRect2(106, 56, 22, 8, BLACK);
    arduboy.drawBitmap(107, 57, imgSound, 14, 7, WHITE);
    arduboy.drawBitmap(122, 57, imgSoundOffOn[arduboy.audio.enabled()], 6, 7, WHITE);
}
