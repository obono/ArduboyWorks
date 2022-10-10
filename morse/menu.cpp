#include "common.h"

/*  Defines  */

#define MENU_COUNT_MAX  9

/*  Typedefs  */

typedef struct {
    void (*func)(void);
    const __FlashStringHelper *label;
} ITEM_T;

/*  Local Variables  */

static int8_t   menuX, menuY, menuW, menuH;
static int8_t   menuItemCount;
static ITEM_T   menuItemAry[MENU_COUNT_MAX];
static int8_t   menuItemPos;
static bool     isFramed, isInvalidMenu;

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
    if (label != NULL) {
        dprint(F("Add menu items: "));
        dprintln(label);
    }
}

int8_t getMenuItemPos(void)
{
    return menuItemPos;
}

int8_t getMenuItemCount(void)
{
    return menuItemCount;
}

void setMenuCoords(int8_t x, int8_t y, int8_t w, int8_t h, bool f)
{
    menuX = x;
    menuY = y;
    menuW = w;
    menuH = h;
    isFramed = f;
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
    setMenuCoords(40, y, 47, 11, true);
    setMenuItemPos(1);
    isInvalidMenu = true;
}

void handleMenu(void)
{
    if (padY != 0) {
        do {
            menuItemPos = circulate(menuItemPos, padY, menuItemCount);
        } while (menuItemAry[menuItemPos].func == NULL);
        playSoundTick();
        isInvalidMenu = true;
        dprint(F("menuItemPos="));
        dprintln(menuItemPos);
    }
    if (arduboy.buttonDown(B_BUTTON)) {
        menuItemAry[menuItemPos].func();
    }
}

void drawMenuItems(bool isForced)
{
    if (!isInvalidMenu && !isForced) return;
    arduboy.fillRect(menuX - 1, menuY - 1, menuW + 2, menuH + 2, BLACK);
    if (isFramed) {
        arduboy.drawRect(menuX - 2, menuY - 2, menuW + 4, menuH + 4, WHITE);
    }
    ITEM_T *pItem = menuItemAry;
    int8_t y = menuY;
    for (int i = 0; i < menuItemCount; i++, pItem++) {
        if (pItem->label != NULL) {
            arduboy.printEx(menuX + 12 - (i == menuItemPos) * 4, y, pItem->label);
            if (i == menuItemPos) arduboy.fillRect(menuX, y, 5, 5, WHITE);
            y += 6;
        } else {
            y += 2;
        }
    }
    isInvalidMenu = false;
}
