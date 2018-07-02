#include "common.h"

/*  Defines  */

#define TITLE_COUNT_MAX  4

enum STATE_T {
    STATE_INIT = 0,
    STATE_TITLE,
    STATE_RECORD,
    STATE_CREDIT,
};

/*  Typedefs  */

typedef struct
{
    MODE_T (*func)(void);
    const __FlashStringHelper *label;
} ITEM_T;

/*  Local Functions  */

static void     setupTitleItems(void);
static void     addTitleItem(const __FlashStringHelper *label, MODE_T (*func)(void));

static MODE_T   handleButtons(void);
static void     handleWaiting(void);

static MODE_T   onStart(void);
static MODE_T   onSound(void);
static MODE_T   onRecord(void);
static MODE_T   onCredit(void);

static void     drawTitleItems(void);
static void     drawTitleItemOnOff(bool on);
static void     drawRecord(void);
static void     drawCredit(void);

/*  Local Variables  */

/*PROGMEM static const uint8_t imgTitle[512] = { // 128x32
};*/

PROGMEM static const char creditText[] = "- " APP_TITLE " -\0\0" APP_RELEASED \
        "\0PROGREMMED BY OBONO\0\0THIS PROGRAM IS\0RELEASED UNDER\0THE MIT LICENSE.";

PROGMEM static const byte soundStart[] = {
    0x90, 72, 0, 100, 0x80, 0, 25,
    0x90, 74, 0, 100, 0x80, 0, 25,
    0x90, 76, 0, 100, 0x80, 0, 25,
    0x90, 77, 0, 100, 0x80, 0, 25,
    0x90, 79, 0, 200, 0x80, 0xF0
};

static STATE_T  state = STATE_INIT;
static bool     toDraw;
static bool     toDrawImage;
static int8_t   itemCount;
static ITEM_T   itemAry[TITLE_COUNT_MAX];
static int8_t   itemPos;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initTitle(void)
{
    if (state == STATE_INIT) {
        readRecord();
        state = STATE_TITLE;
    }
    setupTitleItems();
    itemPos = 0;
    toDraw = true;
    toDrawImage = true;
}

MODE_T updateTitle(void)
{
    MODE_T ret = MODE_TITLE;
    if (state == STATE_RECORD || state == STATE_CREDIT) {
        handleWaiting();
    } else {
        ret = handleButtons();
    }
    randomSeed(rand() ^ micros()); // Shuffle random
    return ret;
}

void drawTitle(void)
{
    if (toDraw) {
        switch (state) {
        case STATE_RECORD:
            drawRecord();
            break;
        case STATE_CREDIT:
            drawCredit();
            break;
        default:
            drawTitleItems();
        }
        toDraw = false;
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void setupTitleItems(void)
{
    itemCount = 0;
    addTitleItem(F("START GAME"), onStart);
    addTitleItem(F("SOUND"), onSound);
    addTitleItem(F("RECORD"), onRecord);
    addTitleItem(F("CREDIT"), onCredit);
}

static void addTitleItem(const __FlashStringHelper *label, MODE_T (*func)(void))
{
    ITEM_T *pItem = &itemAry[itemCount];
    pItem->label = label;
    pItem->func = func;
    itemCount++;
}

static MODE_T handleButtons()
{
    MODE_T ret = MODE_TITLE;
    if (arduboy.buttonDown(UP_BUTTON) && itemPos > 0) {
        itemPos--;
        playSoundTick();
        toDraw = true;
    }
    if (arduboy.buttonDown(DOWN_BUTTON) && itemPos < itemCount - 1) {
        itemPos++;
        playSoundTick();
        toDraw = true;
    }
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        ret = itemAry[itemPos].func();
    }
    return ret;
}

static void handleWaiting(void)
{
    if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        playSoundClick();
        state = STATE_TITLE;
        toDraw = true;
        toDrawImage = true;
    }
}

static MODE_T onStart(void)
{
    arduboy.playScore2(soundStart, 0);
    return MODE_GAME;
}

static MODE_T onSound(void)
{
    setSound(!arduboy.audio.enabled());
    playSoundClick();
    toDraw = true;
    isDirty = true;
    dprint(F("isSoundEnable="));
    dprintln(arduboy.audio.enabled());
    return MODE_TITLE;
}

static MODE_T onRecord(void)
{
    playSoundClick();
    state = STATE_RECORD;
    toDraw = true;
    dprintln(F("Show recoed"));
    return MODE_TITLE;
}

static MODE_T onCredit(void)
{
    playSoundClick();
    state = STATE_CREDIT;
    toDraw = true;
    dprintln(F("Show credit"));
    return MODE_TITLE;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawTitleItems(void)
{
    /*  Title image  */
    uint8_t itemsHeight = itemCount * 6;
    if (toDrawImage) {
        arduboy.clear();
        arduboy.printEx(0, 0, F(APP_TITLE));
        arduboy.printEx(0, 6, F("TITLE SCREEN"));
        //arduboy.drawBitmap(0, 0, imgTitle, 128, 32, WHITE);
        toDrawImage = false;
    } else {
        arduboy.fillRect(56, 40, 72, itemsHeight, BLACK);
    }

    /*  Items  */
    ITEM_T *pItem = itemAry;
    for (int i = 0; i < itemCount; i++, pItem++) {
        arduboy.printEx(68 - (i == itemPos) * 4, i * 6 + 40, pItem->label);
        if (pItem->func == onSound) {
            drawTitleItemOnOff(arduboy.audio.enabled());
        }
    }
    arduboy.fillRect2(56, itemPos * 6 + 40, 5, 5, WHITE);
}

static void drawTitleItemOnOff(bool on)
{
    arduboy.print((on) ? F(" ON") : F(" OFF"));
}

static void drawRecord(void)
{
    arduboy.clear();
    arduboy.printEx(0, 0, F("RECORD"));
}

static void drawCredit(void)
{
    arduboy.clear();
    const char *p = creditText;
    for (int i = 0; i < 8; i++) {
        uint8_t len = strnlen_P(p, 20);
        arduboy.printEx(64 - len * 3, i * 6 + 8, (const __FlashStringHelper *) p);
        p += len + 1;
    }
}
