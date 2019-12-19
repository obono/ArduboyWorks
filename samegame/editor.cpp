#include "common.h"

/*  Defines  */

#define IMG_BUTTONS_W   7
#define IMG_BUTTONS_H   16

#define NO_COLOR        2

enum STATE_T {
    STATE_SELECTING = 0,
    STATE_EDITING,
    STATE_MENU,
    STATE_LEAVE,
};

/*  Local Functions  */

static void handleEditing(void);
static void handleSelecting(void);
static void onContinue(void);
static void onConfirmDiscard(void);
static void onDiscard(void);
static void onConfirmRestore(void);
static void onRestore(void);
static void onConfirmQuit(void);
static void onQuit(void);
static void drawEditorAll(void);
static void drawEditorTargetCursor(void);
static void drawEditorGridCell(int8_t x, int8_t y, bool isFullUpdate);
static void drawEditorGridCursor(bool isFullUpdate);
static void drawEditorCursorActual(int16_t dx, int16_t dy, uint8_t size);

/*  Local Variables  */

PROGMEM static const uint8_t imgButtons[] = { // 7x16
    0x7C, 0x8E, 0xD6, 0xDA, 0xDA, 0x82, 0x7C, 0x7C, 0x82, 0xAA, 0xAA, 0xA2, 0xCA, 0x7C
};

PROGMEM static const byte soundCancel[] = {
    0x90, 0x48, 0x00, 0x28, 0x90, 0x45, 0x00, 0x28, 0x90, 0x41, 0x00, 0x28, 0x80, 0xF0
};

static STATE_T  state;
static uint8_t  backup[16];
static uint8_t  counter, targetObject, cursorX, cursorY, drawColor;

/*  Local Functions (macros)  */

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))
#define calcBit(t, x, y)    ((t) * IMG_OBJECT_W * IMG_OBJECT_H + (x) * IMG_OBJECT_H + (y))
#define getPattern(bit)     bitRead(record.imgObject[bit >> 3], bit & 7)
#define setPattern(bit, v)  ((v) ? bitSet(record.imgObject[bit >> 3], bit & 7) : \
                                    bitClear(record.imgObject[bit >> 3], bit & 7));

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initEditor(void)
{
    memcpy(backup, record.imgObject, sizeof(backup));
    targetObject = 0;
    cursorX = IMG_OBJECT_W / 2;
    cursorY = IMG_OBJECT_H / 2;
    state = STATE_SELECTING;
    isInvalid = true;
}

MODE_T updateEditor(void)
{
    counter++;
    handleDPad();
    switch (state) {
    case STATE_SELECTING:
        handleSelecting();
        break;
    case STATE_EDITING:
        handleEditing();
        break;
    case STATE_MENU:
        handleMenu();
        break;
    default:
        break;
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_EDITOR;
}

void drawEditor(void)
{
    if (state == STATE_LEAVE) return;
    if (state == STATE_MENU) {
        if (isInvalid) arduboy.fillRect2(85, 49, 43, 15, BLACK);
        drawMenuItems(isInvalid);
        isInvalid = false;
        return;
    }
    if (isInvalid) {
        arduboy.clear();
        drawEditorAll();
    }
    if (state == STATE_SELECTING) drawEditorTargetCursor();
    if (state == STATE_EDITING) drawEditorGridCursor(isInvalid);
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handleSelecting(void)
{
    if (padY != 0) {
        playSoundTick();
        targetObject = circulate(targetObject, padY, OBJECT_TYPES);
        isInvalid = true;
    }
    if (arduboy.buttonDown(B_BUTTON)) {
        playSoundClick();
        drawColor = NO_COLOR;
        state = STATE_EDITING;
        isInvalid = true;
    } else if (arduboy.buttonDown(A_BUTTON)) {
        drawColor = NO_COLOR;
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("CONTINUE"), onContinue);
        addMenuItem(F("BACK TO TITLE"), (isRecordDirty) ? onConfirmQuit : onQuit);
        if (isRecordDirty) addMenuItem(F("DISCARD CHANGES"), onConfirmDiscard);
        addMenuItem(F("RESTORE DEFAULT"), onConfirmRestore);
        setMenuCoords(13, 20, 101, getMenuItemCount() * 6 - 1, true, true);
        setMenuItemPos(0);
        state = STATE_MENU;
        isInvalid = true;
    }
}

static void handleEditing(void)
{
    if (padX != 0 || padY != 0) {
        playSoundTick();
        cursorX = circulate(cursorX, padX, IMG_OBJECT_W);
        cursorY = circulate(cursorY, padY, IMG_OBJECT_H);
    }
    uint8_t bit = calcBit(targetObject, cursorX, cursorY);
    if (arduboy.buttonDown(B_BUTTON)) {
        playSoundTick();
        drawColor = getPattern(bit) ? BLACK : WHITE;
    }
    if (arduboy.buttonPressed(B_BUTTON) && drawColor != NO_COLOR) {
        isRecordDirty = true;
        setPattern(bit, drawColor);
    }
    if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        state = STATE_SELECTING;
        isInvalid = true;
    }
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onContinue(void)
{
    playSoundClick();
    state = STATE_SELECTING;
    isInvalid = true;
    dprintln(F("Menu: continue"));
}

static void onConfirmDiscard(void)
{
    int16_t y = getMenuItemPos() * 6 + 22;
    setConfirmMenu(y, onDiscard, onContinue);
    dprintln(F("Menu: confirm discard"));
}

static void onDiscard(void)
{
    arduboy.playScore2(soundCancel, 0);
    memcpy(record.imgObject, backup, sizeof(backup));
    isRecordDirty = false;
    state = STATE_SELECTING;
    isInvalid = true;
    dprintln(F("Menu: discard"));
}

static void onConfirmRestore(void)
{
    int16_t y = getMenuItemPos() * 6 + 22;
    setConfirmMenu(y, onRestore, onContinue);
    dprintln(F("Menu: confirm restore"));
}

static void onRestore(void)
{
    arduboy.playScore2(soundCancel, 0);
    restoreObjectImage();
    isRecordDirty = memcmp(record.imgObject, backup, sizeof(backup));
    state = STATE_SELECTING;
    isInvalid = true;
    dprintln(F("Menu: restore"));
}

static void onConfirmQuit(void)
{
    int16_t y = getMenuItemPos() * 6 + 22;
    setConfirmMenu(y, onQuit, onContinue);
    dprintln(F("Menu: confirm quit"));
}

static void onQuit(void)
{
    playSoundClick();
    writeRecord();
    state = STATE_LEAVE;
    dprintln(F("Menu: quit"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawEditorAll(void)
{
    arduboy.printEx(15, 0, F("PATTERN EDITOR"));

    /*  Objects  */
    for (int8_t i = 0; i < OBJECT_TYPES; i++) {
        drawObject(10, i * 10 + 10, i);
    }
    arduboy.drawRect2(7, targetObject * 10 + 7, 11, 11, WHITE);

    /*  Grid  */
    for (int8_t y = 0; y < IMG_OBJECT_H; y++) {
        for (int8_t x = 0; x < IMG_OBJECT_W; x++) {
            drawEditorGridCell(x, y, true);
        }
    }
    for (int8_t i = 0; i <= IMG_OBJECT_H; i++) {
        arduboy.drawFastHLine2(31, i * 10 + 7, 51, WHITE);
        arduboy.drawFastVLine2(i * 10 + 31, 7, 51, WHITE);
    }

    /*  Instruction  */
    arduboy.drawBitmap(85, 48, imgButtons, IMG_BUTTONS_W, IMG_BUTTONS_H, WHITE);
    bool isSelecting = (state == STATE_SELECTING);
    arduboy.printEx(93, 50, (isSelecting) ? F("MENU") : F("SELECT"));
    arduboy.printEx(93, 58, (isSelecting) ? F("DESIGN") : F("DOT"));
}

static void drawEditorTargetCursor(void)
{
    drawEditorCursorActual(6, targetObject * 10 + 6, 13);
}

static void drawEditorGridCell(int8_t x, int8_t y, bool isFullUpdate)
{
    uint8_t bit = calcBit(targetObject, x, y);
    uint8_t color = getPattern(bit);
    if (color == WHITE || !isFullUpdate) {
        arduboy.fillRect2(x * 10 + 32, y * 10 + 8, 9, 9, color);
    }
}

static void drawEditorGridCursor(bool isFullUpdate)
{
    static int8_t lastX, lastY;
    if (!isFullUpdate) {
        arduboy.fillRect2(10, targetObject * 10 + 10, IMG_OBJECT_W, IMG_OBJECT_H, BLACK);
        drawObject(10, targetObject * 10 + 10, targetObject);
        drawEditorGridCell(lastX, lastY, false);
    }
    drawEditorCursorActual(cursorX * 10 + 32, cursorY * 10 + 8, 9);
    lastX = cursorX;
    lastY = cursorY;
}

static void drawEditorCursorActual(int16_t dx, int16_t dy, uint8_t size)
{
    uint8_t color = !((counter >> 3) & 3);
    arduboy.drawRect2(dx, dy, size, size, color);
    arduboy.drawRect2(dx + 1, dy + 1, size - 2, size - 2, !color);
}
