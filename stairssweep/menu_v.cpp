#include "common.h"

/*  Defines  */

#define MENU_COUNT_MAX  7

/*  Typedefs  */

typedef struct
{
    void (*func)(void);
    const __FlashStringHelper *label;
} ITEM_T;

/*  Local Variables  */

PROGMEM static const uint8_t imgSound[14] = { // 7x15
    0x02, 0x06, 0x6E, 0x6E, 0x6E, 0x06, 0x02, 0x3E, 0x71, 0x6D, 0x5D, 0x41, 0x5D, 0x3E
};

PROGMEM static const uint8_t imgSoundOffOn[2][7] = { // 7x6 x2
    { 0x00, 0x00, 0x14, 0x08, 0x14, 0x00, 0x00 },
    { 0x02, 0x09, 0x25, 0x15, 0x25, 0x09, 0x02 },
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
    dprintln((label != NULL) ? label : F("(null)"));
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

void handleMenu(void)
{
    if (arduboy.buttonDown(UP_BUTTON_V) && menuItemPos > 0) {
        while (!(menuItemAry[--menuItemPos].func)) { ; }
        playSoundTick();
        isInvalidMenu = true;
        dprint(F("menuItemPos="));
        dprintln(menuItemPos);
    }
    if (arduboy.buttonDown(DOWN_BUTTON_V) && menuItemPos < menuItemCount - 1) {
        while (!(menuItemAry[++menuItemPos].func)) { ; }
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
    arduboy.fillRect2(menuX - 1, menuY - menuH, menuW + 2, menuH + 2, BLACK);
    if (isFramed) {
        arduboy.drawRect2(menuX - 2, menuY - menuH - 1, menuW + 4, menuH + 4, WHITE);
    }
    ITEM_T *pItem = menuItemAry;
    int16_t x = menuX;
    for (int i = 0; i < menuItemCount; i++, pItem++) {
        if (pItem->label) {
            int16_t y = menuY;
            if (pItem->func) y -= 12;
            if (i == menuItemPos) {
                arduboy.fillRect2(x, menuY - 5, 5, 5, WHITE);
                y += 4;
            }
            arduboy.printEx(x, y, pItem->label);
            x += 6;
        } else {
            x += 2;
        }
    }
    if (isControlSound) drawSoundEnabled();
    isInvalidMenu = false;
}

void drawSoundEnabled(void)
{
    arduboy.fillRect2(120, 0, 8, 22, BLACK);
    arduboy.drawBitmap(121, 0, imgSoundOffOn[arduboy.audio.enabled()], 7, 6, WHITE);
    arduboy.drawBitmap(121, 6, imgSound, 7, 15, WHITE);
}
