#include "common.h"
#include "data.h"

/*  Defines  */

#define FIELD_W     20
#define FIELD_H     10

#define getField(x, y)      ((field[x] >> (y) * 3) & 7)
#define getLinkedFlag(x, y) (bitRead(linkedFlag[x], y))

enum STATE_T {
    STATE_INIT = 0,
    STATE_PLAYING,
    STATE_MENU,
    STATE_OVER,
    STATE_LEAVE,
};

/*  Typedefs  */

/*  Local Functions  */

static void     checkLinkedObjects(int8_t x, int8_t y);
static void     checkLinkedObjectsInColumn(int8_t x, int8_t y1, int8_t y2);
static void     removeLinkedObjects(void);
static void     closeField(void);
static uint32_t closeFiledInColumn(int8_t x);

static void     onContinue(void);
static void     onRetry(void);
static void     onQuit(void);
static void     onExit(void);

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static uint32_t field[FIELD_W];
static uint16_t linkedFlag[FIELD_W];
static uint16_t score;
static int8_t   cursorX, cursorY, fieldWidth;
static uint8_t  counter, linkedTarget, linkedCount;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    arduboy.playScore2(soundStart, SND_PRIO_START);
    for (uint8_t x = 0; x < FIELD_W; x++) {
        uint32_t value = 0;
        for (uint8_t y = 0; y < FIELD_H; y++) {
            value |= random(1, 6) << y * 3;
        }
        field[x] = value;
    }
    fieldWidth = FIELD_W;
    cursorX = FIELD_W / 2;
    cursorY = FIELD_H / 2;
    score = 0;
    checkLinkedObjects(cursorX, cursorY);
    record.playCount++;
    isRecordDirty = true;
    writeRecord();
    state = STATE_PLAYING;
    isInvalid = true;
}

MODE_T updateGame(void)
{
    counter++;
    handleDPad();
    switch (state) {
    case STATE_PLAYING:
        record.playFrames++;
        isRecordDirty = true;
        if (padX != 0 || padY != 0) {
            playSoundTick();
            cursorX += padX;
            cursorY += padY;
            if (cursorX < 0) cursorX = 0;
            if (cursorX >= fieldWidth) cursorX = fieldWidth - 1;
            if (cursorY < 0) cursorY = 0;
            if (cursorY >= FIELD_H) cursorY = FIELD_H - 1;
            if (!getLinkedFlag(cursorX, cursorY)) checkLinkedObjects(cursorX, cursorY);
            isInvalid = true;
        }
        if (arduboy.buttonDown(A_BUTTON)) {
            playSoundClick();
            clearMenuItems();
            addMenuItem(F("CONTINUE"), onContinue);
            addMenuItem(F("RESTART GAME"), onRetry);
            addMenuItem(F("QUIT"), onQuit);
            setMenuCoords(19, 23, 89, 17, true, true);
            setMenuItemPos(0);
            state = STATE_MENU;
            isInvalid = true;
        } else if (arduboy.buttonDown(B_BUTTON) && linkedCount > 1) {
            playSoundClick();
            removeLinkedObjects();
            score += (linkedCount - 2) * (linkedCount - 2);
            closeField();
            checkLinkedObjects(cursorX, cursorY);
            /*arduboy.playScore2(soundOver, SND_PRIO_OVER);
            writeRecord();
            state = STATE_OVER;*/
            isInvalid = true;
        }
        break;
    case STATE_OVER:
        if (arduboy.buttonDown(A_BUTTON)) {
            playSoundClick();
            clearMenuItems();
            addMenuItem(F("RETRY GAME"), onRetry);
            addMenuItem(F("BACK TO TITLE"), onExit);
            setMenuCoords(19, 26, 89, 11, true, false);
            setMenuItemPos(0);
            state = STATE_MENU;
            isInvalid = true;
        }
        break;
    case STATE_MENU:
        handleMenu();
        break;
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (state == STATE_LEAVE) return;
    if (state == STATE_MENU) {
        drawMenuItems(isInvalid);
        isInvalid = false;
        return;
    }
    if (isInvalid) {
        arduboy.clear();
        int16_t offsetX = 64 - fieldWidth * 3;
        for (uint8_t x = 0; x < fieldWidth; x++) {
            for (uint8_t y = 0; y < FIELD_H; y++) {
                uint8_t value = getField(x, y);
                if (value) {
                    arduboy.drawBitmap(x * 6 + offsetX, y * 6 + 4, imgObject[value], IMG_OBJECT_W, IMG_OBJECT_H, WHITE);
                    if (getLinkedFlag(x, y)) {
                        arduboy.drawFastHLine2(x * 6 + offsetX + 1, y * 6 + 9, 3, WHITE);
                    }
                }
            }
        }
        arduboy.drawRect2(cursorX * 6 + offsetX - 1, cursorY * 6 + 3, 7, 7, WHITE);
        drawNumber(0, 0, score);
        if (state == STATE_OVER) {
            arduboy.printEx(37, 29, F("GAME OVER"));
        }
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

void checkLinkedObjects(int8_t x, int8_t y)
{
    memset(linkedFlag, 0, sizeof(linkedFlag));
    linkedTarget = getField(x, y);
    linkedCount = 0;
    if (linkedTarget) checkLinkedObjectsInColumn(x, y, y + 1);
}

void checkLinkedObjectsInColumn(int8_t x, int8_t y1, int8_t y2)
{
    if (x < 0 || x > FIELD_W) return;
    while (y1 >= 0 && !getLinkedFlag(x, y1) && getField(x, y1) == linkedTarget) y1--;
    y1++;
    while (y1 < y2) {
        if (!getLinkedFlag(x, y1) && getField(x, y1) == linkedTarget) {
            int8_t y3 = y1;
            do {
                bitSet(linkedFlag[x], y3);
                linkedCount++;
                y3++;
            } while (y3 < FIELD_H && !getLinkedFlag(x, y3) && getField(x, y3) == linkedTarget);
            checkLinkedObjectsInColumn(x - 1, y1, y3);
            checkLinkedObjectsInColumn(x + 1, y1, y3);
            y1 = y3;
        } else {
            y1++;
        }
    }
}

void removeLinkedObjects(void)
{
    for (int8_t x = 0; x < FIELD_W; x++) {
        uint32_t value = field[x];
        for (int8_t y = 0; y < FIELD_H; y++) {
            if (bitRead(linkedFlag[x], y)) {
                value &= ~(7UL << y * 3);
            }
        }
        field[x] = value;
    }
}

void closeField(void)
{
    uint8_t destX = 0;
    for (int8_t srcX = 0; srcX < fieldWidth; srcX++) {
        if (srcX == cursorX) cursorX = destX;
        uint32_t value = closeFiledInColumn(srcX);
        if (value) {
            field[destX] = value;
            destX++;
        }
    }
    memset(field + destX, 0, (fieldWidth - destX) * sizeof(*field));
    fieldWidth = destX;
}

uint32_t closeFiledInColumn(int8_t x)
{
    if (!field[x]) return 0;
    uint32_t value = 0;
    for (int8_t srcY = FIELD_H - 1, destY = FIELD_H - 1; srcY >= 0; srcY--) {
        uint8_t object = getField(x, srcY);
        if (object) {
            value |= (uint32_t) object << destY * 3;
            destY--;
        }
    }
    return value;
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onContinue(void)
{
    playSoundClick();
    state = STATE_PLAYING;
    isInvalid = true;
    dprintln(F("Menu: continue"));
}

static void onRetry(void)
{
    dprintln(F("Menu: retry"));
    initGame();
}

static void onQuit(void)
{
    playSoundClick();
    clearMenuItems();
    addMenuItem(F("QUIT"), onExit);
    addMenuItem(F("CANCEL"), onContinue);
    setMenuCoords(40, 38, 47, 11, true, true);
    setMenuItemPos(1);
    isInvalid = true;
    dprintln(F("Menu: quit"));
}

static void onExit(void)
{
    playSoundClick();
    writeRecord();
    state = STATE_LEAVE;
    dprintln(F("Menu: exit"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/
