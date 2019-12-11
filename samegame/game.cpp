#include "common.h"
#include "data.h"

/*  Defines  */

#define FIELD_W         20
#define FIELD_H         10
#define GRID_SIZE       6
#define OBJECT_TYPES    5
#define ERASING_ANIM    5
#define PERFECT_BONUS   1000

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))
#define getField(x, y)      ((field.objects[x] >> (y) * 3) & 7)
#define getLinkedFlag(x, y) (bitRead(linkedFlag[x], y))
#define getOffsetX()        (WIDTH / 2 - fieldWidth * GRID_SIZE / 2)

enum STATE_T {
    STATE_INIT = 0,
    STATE_PLAYING,
    STATE_ERASING,
    STATE_OVER,
    STATE_MENU,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    uint32_t    objects[FIELD_W];
    uint16_t    score;
    int8_t      cursorX, cursorY;
} FIELD_T;

/*  Local Functions  */

static void     handlePlaying(void);
static void     handleErasing(void);
static void     handleOver(void);

static void     initField(void);
static void     checkLinkedObjects(void);
static void     checkLinkedObjectsInColumn(int8_t x, int8_t y1, int8_t y2);
static void     removeLinkedObjects(void);
static bool     closeField(void);
static uint32_t closeFieldInColumn(int8_t x);
static bool     judgeGameOver(void);
static void     backupField(void);
static void     restoreField(void);

static void     onContinue(void);
static void     onUndo(void);
static void     onRetryConfirm(void);
static void     onRetry(void);
static void     onQuitConfirm(void);
static void     onQuit(void);
static void     onConfirm(void (*func)());

static void     drawField(bool isFullUpdate);
static void     drawScore(void);
static void     drawCursor(void);
static void     drawCursorActual(int8_t x, int8_t y, uint8_t imgId);
static void     drawEraser(void);
static void     drawResult(bool isFullUpdate);
static void     drawResultLabel(int16_t dy, uint8_t anim);
static void     drawResultScore(int16_t dy, uint8_t anim);
static void     drawResultNewRecord(int16_t dy);

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static FIELD_T  field, lastField;
static uint16_t linkedFlag[FIELD_W], displayScore;
static uint8_t  counter, fieldWidth, linkedTarget, linkedCount;
static bool     isTouched, isPerfect, isHiscore, isBackable;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    arduboy.playScore2(soundStart, SND_PRIO_START);
    initField();
    isBackable = false;
    lastScore = 0;
    displayScore = 0;
    state = STATE_PLAYING;
    isInvalid = true;
}

MODE_T updateGame(void)
{
    counter++;
    if (state == STATE_PLAYING || state == STATE_ERASING) {
        record.playFrames++;
        isRecordDirty = true;
        if (displayScore < field.score) {
            displayScore += (field.score - displayScore + 63) >> 6;
        }
    }
    handleDPad();
    switch (state) {
    case STATE_PLAYING:
        handlePlaying();
        break;
    case STATE_ERASING:
        handleErasing();
        break;
    case STATE_OVER:
        handleOver();
        break;
    case STATE_MENU:
        handleMenu();
        break;
    default:
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
        drawField(true);
    }
    switch (state) {
    case STATE_PLAYING:
        if (!isInvalid) drawField(false);
        drawCursor();
        drawScore();
        break;
    case STATE_ERASING:
        drawEraser();
        drawScore();
        break;
    case STATE_OVER:
        drawResult(isInvalid);
        break;
    default:
        break;
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void handlePlaying(void)
{
    if (padX != 0 || padY != 0) {
        playSoundTick();
        field.cursorX = circulate(field.cursorX, padX, fieldWidth);
        field.cursorY = circulate(field.cursorY, padY, FIELD_H);
        if (!getLinkedFlag(field.cursorX, field.cursorY)) {
            if (linkedCount > 1) isInvalid = true;
            checkLinkedObjects();
        }
    }
    if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("CONTINUE"), onContinue);
        if (isBackable) addMenuItem(F("UNDO LAST MOVE"), onUndo);
        addMenuItem(F("RESTART GAME"), (isTouched) ? onRetryConfirm : onRetry);
        addMenuItem(F("BACK TO TITLE"), (isTouched) ? onQuitConfirm : onQuit);
        setMenuCoords(16, 20, 95, (isBackable) ? 23 : 17, true, true);
        setMenuItemPos((isTouched) ? 0 : 1);
        state = STATE_MENU;
        isInvalid = true;
    } else if (linkedCount > 1 && arduboy.buttonDown(B_BUTTON)) {
        const byte *pSound;
        if (linkedCount < 20) {
            pSound = (const byte *)pgm_read_ptr(soundEraseTable + linkedCount / 5);
        } else {
            pSound = soundErase4;
        }
        arduboy.playScore2(pSound, SND_PRIO_EFFECTS);
        backupField();
        if (!isTouched) {
            isTouched = true;
            record.playCount++;
            writeRecord();
            dprintln(F("Increase play count"));
        }
        uint16_t obtainedScore = (linkedCount - 2) * (linkedCount - 2);
        dprint(obtainedScore);
        dprintln(F(" points"));
        field.score += obtainedScore;
        counter = 0;
        state = STATE_ERASING;
        isInvalid = true;
    }
#ifdef DEBUG
    if (dbgRecvChar == 'a' && linkedCount > 0) {
        counter = 0;
        state = STATE_ERASING;
        isInvalid = true;
    }
    if (dbgRecvChar == 'x') {
        for (int8_t x = 0; x < fieldWidth; x++) {
            linkedFlag[x] = (1 << FIELD_H) - 1;
        }
        linkedCount = fieldWidth * FIELD_H;
        counter = 0;
        state = STATE_ERASING;
        isInvalid = true;
    }
    if (dbgRecvChar == 'z') {
        isPerfect = false;
        isHiscore = enterScore(field.score);
        writeRecord();
        counter = 0;
        state = STATE_OVER;
    }
#endif
}

static void handleErasing(void)
{
    if (counter < linkedCount + ERASING_ANIM * 2 - 1) return;

    removeLinkedObjects();
    if (closeField()) arduboy.playScore2(soundFall, SND_PRIO_EFFECTS);
    if (judgeGameOver()) {
        if (fieldWidth == 0) {
            arduboy.playScore2(soundPerfect, SND_PRIO_OVER);
            isPerfect = true;
            field.score += PERFECT_BONUS;
            dprintln(F("Perfect!"));
        } else {
            arduboy.playScore2(soundOver, SND_PRIO_OVER);
            isPerfect = false;
            dprintln(F("No moves..."));
        }
        isHiscore = enterScore(field.score);
        writeRecord();
        counter = 0;
        state = STATE_OVER;
    } else {
        checkLinkedObjects();
        state = STATE_PLAYING;
    }
    isInvalid = true;
}

static void handleOver(void)
{
    if (counter < FPS * 2) return;

    if (counter == FPS * 2 && isPerfect) arduboy.stopScore2();
    if (counter >= 192) counter -= 16;
    if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("RETRY GAME"), onRetry);
        addMenuItem(F("BACK TO TITLE"), onQuit);
        setMenuCoords(19, 50, 89, 11, true, false);
        setMenuItemPos(0);
        state = STATE_MENU;
        isInvalid = true;
    } else if (!isPerfect && (arduboy.buttonDown(B_BUTTON) || arduboy.buttonUp(B_BUTTON))) {
        isInvalid = true;
    }
}

static void initField(void)
{
    for (int8_t x = 0; x < FIELD_W; x++) {
        uint32_t value = 0;
        for (int8_t y = 0; y < FIELD_H; y++) {
            value |= random(1, OBJECT_TYPES + 1) << y * 3;
        }
        field.objects[x] = value;
    }
    field.score = 0;
    field.cursorX = FIELD_W / 2;
    field.cursorY = FIELD_H / 2;
    fieldWidth = FIELD_W;
    isTouched = false;
    checkLinkedObjects();
}

static void checkLinkedObjects(void)
{
    int8_t x = field.cursorX, y = field.cursorY;
    memset(linkedFlag, 0, sizeof(linkedFlag));
    linkedTarget = getField(x, y);
    linkedCount = 0;
    if (linkedTarget) checkLinkedObjectsInColumn(x, y, y + 1);
}

static void checkLinkedObjectsInColumn(int8_t x, int8_t y1, int8_t y2)
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

static void removeLinkedObjects(void)
{
    for (int8_t x = 0; x < fieldWidth; x++) {
        uint32_t value = field.objects[x];
        for (int8_t y = 0; y < FIELD_H; y++) {
            if (bitRead(linkedFlag[x], y)) {
                value &= ~(7UL << y * 3);
            }
        }
        field.objects[x] = value;
    }
    dprint(F("Remove "));
    dprint(linkedCount);
    dprintln(F(" objects"));
}

static bool closeField(void)
{
    bool isFalled = false;
    int8_t destX = 0;
    for (int8_t srcX = 0; srcX < fieldWidth; srcX++) {
        if (srcX == field.cursorX) field.cursorX = destX;
        uint32_t lastValue = field.objects[srcX];
        uint32_t value = closeFieldInColumn(srcX);
        if (value) {
            field.objects[destX] = value;
            destX++;
            if (value != lastValue) isFalled = true;
        }
    }
    if (destX < fieldWidth) {
        memset(field.objects + destX, 0, (fieldWidth - destX) * sizeof(*field.objects));
        fieldWidth = destX;
        isFalled = true;
    }
    dprint(F("Field width="));
    dprintln(fieldWidth);
    return isFalled;
}

static uint32_t closeFieldInColumn(int8_t x)
{
    if (!field.objects[x]) return 0;
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

static bool judgeGameOver(void)
{
    for (int8_t x = 0; x < fieldWidth; x++) {
        uint8_t lastObject = 0;
        for (int8_t y = 0; y < FIELD_H; y++) {
            uint8_t object = getField(x, y);
            if (object) {
                if (object == lastObject || x < fieldWidth - 1 && object == getField(x + 1, y)) {
                    return false;
                }
            }
            lastObject = object;
        }
    }
    return true;
}

static void backupField(void)
{
    memcpy(&lastField, &field, sizeof(field));
    isBackable = true;
}

static void restoreField(void)
{
    memcpy(&field, &lastField, sizeof(field));
    while (fieldWidth < FIELD_W && field.objects[fieldWidth]) {
        fieldWidth++;
    }
    checkLinkedObjects();
    displayScore = field.score;
    isBackable = false;
    dprintln(F("Restore field"));
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

static void onUndo(void)
{
    arduboy.playScore2(soundUndo, SND_PRIO_EFFECTS);
    restoreField();
    state = STATE_PLAYING;
    isInvalid = true;
    dprintln(F("Menu: undo"));
}

static void onRetryConfirm(void)
{
    onConfirm(onRetry);
    dprintln(F("Menu: retry confirm"));
}

static void onRetry(void)
{
    dprintln(F("Menu: retry"));
    initGame();
}

static void onQuitConfirm(void)
{
    onConfirm(onQuit);
    dprintln(F("Menu: quit confirm"));
}

static void onQuit(void)
{
    playSoundClick();
    writeRecord();
    state = STATE_LEAVE;
    dprintln(F("Menu: quit"));
}

static void onConfirm(void (*func)())
{
    playSoundClick();
    clearMenuItems();
    addMenuItem(F("OK"), func);
    addMenuItem(F("CANCEL"), onContinue);
    setMenuCoords(40, 34, 47, 11, true, true);
    setMenuItemPos(1);
    isInvalid = true;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawField(bool isFullUpdate)
{
    if (!isFullUpdate && linkedCount <= 1) return;
    int16_t offsetX = getOffsetX();
    uint8_t blinkFrac = counter & 7;
    for (int8_t x = 0; x < fieldWidth; x++) {
        for (int8_t y = 0; y < FIELD_H; y++) {
            uint8_t object = getField(x, y);
            if (object) {
                bool isLinked = getLinkedFlag(x, y);
                int16_t dx = x * GRID_SIZE + offsetX;
                int16_t dy = y * GRID_SIZE + 4;
                if (isFullUpdate || isLinked && ((x + y) & 7) == blinkFrac) {
                    arduboy.drawBitmap(dx, dy, imgObject[object],
                            IMG_OBJECT_W, IMG_OBJECT_H, WHITE);
                }
                if (!isFullUpdate && isLinked && ((x + y + 7) & 7) == blinkFrac) {
                    arduboy.fillRect2(dx, dy, IMG_OBJECT_W, IMG_OBJECT_H, BLACK);
                }
            }
        }
    }
}

static void drawScore(void)
{
    drawNumber(0, 0, displayScore);
}

static void drawCursor(void)
{
    static int8_t lastX, lastY;
    drawCursorActual(lastX, lastY, IMG_CURSOR_ID_CLEAR);
    lastX = field.cursorX;
    lastY = field.cursorY;
    uint8_t imgId = counter >> 1 & 15;
    if (imgId >= IMG_CURSOR_ID_CLEAR) imgId = 14 - imgId;
    if (imgId >= IMG_CURSOR_ID_CLEAR) imgId = 0;
    drawCursorActual(lastX, lastY, imgId);
}

static void drawCursorActual(int8_t x, int8_t y, uint8_t imgId)
{
    int16_t dx = x * GRID_SIZE + getOffsetX() - 1;
    int16_t dy = y * GRID_SIZE + 3;
    uint8_t color = (imgId == IMG_CURSOR_ID_CLEAR) ? BLACK : WHITE;
    arduboy.drawBitmap(dx, dy, imgCursor[imgId], IMG_CURSOR_W, IMG_CURSOR_H, color);
}

static void drawEraser(void)
{
    int16_t offsetX = getOffsetX();
    for (int8_t x = 0; x < fieldWidth; x++) {
        for (int8_t y = 0; y < FIELD_H; y++) {
            if (getLinkedFlag(x, y)) {
                int16_t dx = x * GRID_SIZE + offsetX;
                int16_t dy = y * GRID_SIZE + 4;
                int8_t anim = counter - abs(field.cursorX - x) - abs(field.cursorY - y);
                if (anim >= 0 && anim < ERASING_ANIM) {
                    arduboy.drawBitmap(dx, dy, imgEraser[anim], IMG_OBJECT_W, IMG_OBJECT_H, WHITE);
                } else if (anim >= ERASING_ANIM && anim < ERASING_ANIM * 2) {
                    arduboy.drawBitmap(dx, dy, imgEraser[anim - ERASING_ANIM],
                            IMG_OBJECT_W, IMG_OBJECT_H, BLACK);
                }
            }
        }
    }
}

static void drawResult(bool isFullUpdate)
{
    if (counter > FPS * 2 && !isPerfect && arduboy.buttonPressed(B_BUTTON)) return;

    int16_t dy = (isPerfect) ? 14 : 0;
    if (counter >= 0 && (isFullUpdate || counter < FPS)) {
        drawResultLabel(dy, (counter < FPS) ? FPS - 1 - counter : 0);
    }
    if (counter >= FPS && (isFullUpdate || counter < FPS * 2)) {
        drawResultScore(dy + IMG_OVER_H + 2, (counter < FPS * 2) ? FPS * 2 - 1 - counter : 0);
    }
    if (isHiscore && counter >= FPS * 2 && (isFullUpdate || (counter & 7) == 0)) {
        drawResultNewRecord(dy + IMG_OVER_H + IMG_DIGIT_H + 4);
    }
}

static void drawResultLabel(int16_t dy, uint8_t anim)
{
    arduboy.fillRect2(0, dy, WIDTH, IMG_OVER_H + 2, BLACK);
    int16_t dx = anim * anim / 28;
    if (isPerfect) {
        dx += (WIDTH - IMG_PERFECT_W) / 2;
        arduboy.drawBitmap(dx, dy, imgPerfect, IMG_PERFECT_W, IMG_PERFECT_H, WHITE);
    } else {
        dx += (WIDTH - IMG_OVER_W) / 2;
        arduboy.drawBitmap(dx, dy, imgOver, IMG_OVER_W, IMG_OVER_H, WHITE);
    }
}

static void drawResultScore(int16_t dy, uint8_t anim)
{
    arduboy.fillRect2(0, dy, WIDTH, IMG_DIGIT_H + 2, BLACK);
    int16_t dx = anim * anim / 28 + 25;
    arduboy.printEx(dx, dy + 11, F("SCORE"));
    dx += 66;
    uint16_t tmp = field.score;
    while (tmp > 0) {
        arduboy.drawBitmap(dx, dy, imgDigit[tmp % 10], IMG_DIGIT_W, IMG_DIGIT_H, WHITE);
        tmp /= 10;
        dx -= IMG_DIGIT_W;
    }
}

static void drawResultNewRecord(int16_t dy)
{
    arduboy.fillRect2(30, dy, 67, 6, BLACK);
    if (bitRead(counter, 3)) arduboy.printEx(31, dy, F("NEW RECORD!"));
}
