#include "common.h"
#include "data.h"

/*  Defines  */

#define FIELD_W         20
#define FIELD_H         10
#define GRID_SIZE       6
#define ERASING_ANIM    7
#define RESULT_ANIM     45

enum STATE_T {
    STATE_PLAYING = 0,
    STATE_ERASING,
    STATE_OVER,
    STATE_MENU,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    uint32_t    objects[FIELD_W];
    uint16_t    score;
    int8_t      cursorX, cursorY, width, height;
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
static uint32_t closeFieldInColumn(int8_t x, int8_t &h);
static bool     judgeGameOver(void);
static void     backupField(void);
static void     restoreField(void);

static void     onContinue(void);
static void     onUndo(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawField(bool isFullUpdate);
static void     drawScore(void);
static void     drawCursor(void);
static void     drawCursorActual(int8_t x, int8_t y, uint8_t imgId);
static void     drawEraser(void);
static void     drawResult(bool isFullUpdate);
static void     drawResultLabel(int16_t dy, uint8_t anim);
static void     drawResultScore(int16_t dy, uint8_t anim);

/*  Local Functions (macros)  */

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))
#define getField(x, y)      ((field.objects[x] >> (y) * 3) & 7)
#define getLinkedFlag(x, y) (bitRead(linkedFlag[x], y))
#define getOffsetX()        (WIDTH / 2 - field.width * GRID_SIZE / 2)

/*  Local Variables  */

static STATE_T  state;
static FIELD_T  field, lastField;
static uint16_t linkedFlag[FIELD_W], displayScore;
static uint8_t  counter, linkedTarget, linkedCount;
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
        field.cursorX = circulate(field.cursorX, padX, field.width);
        field.cursorY = circulate(field.cursorY, -padY, field.height);
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
        addMenuItem(F("RESTART GAME"), (isTouched) ? onConfirmRetry : onRetry);
        addMenuItem(F("BACK TO TITLE"), (isTouched) ? onConfirmQuit : onQuit);
        setMenuCoords(16, 20, 95, getMenuItemCount() * 6 - 1, true, true);
        setMenuItemPos((isTouched) ? 0 : 1);
        state = STATE_MENU;
        isInvalid = true;
    } else if (linkedCount > 1 && arduboy.buttonDown(B_BUTTON)) {
        if (!isTouched) {
            isTouched = true;
            record.playCount++;
            writeRecord();
            dprintln(F("Increase play count"));
        }
        const byte *pSound;
        if (linkedCount < 20) {
            pSound = (const byte *)pgm_read_ptr(soundEraseTable + linkedCount / 5);
        } else {
            pSound = soundErase4;
        }
        arduboy.playScore2(pSound, SND_PRIO_EFFECTS);
        backupField();
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
        for (int8_t x = 0; x < field.width; x++) {
            linkedFlag[x] = (1 << field.height) - 1;
        }
        linkedCount = field.width * field.height;
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
        if (field.width == 0) {
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

    if (counter >= 240) counter -= 16; // Trick!
    if (arduboy.buttonDown(A_BUTTON) || isPerfect && counter == FPS * 2) {
        playSoundClick();
        if (isPerfect) arduboy.stopScore2();
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
    field.width = FIELD_W;
    field.height = FIELD_H;
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
    for (int8_t x = 0; x < field.width; x++) {
        uint32_t value = field.objects[x];
        for (int8_t y = 0; y < field.height; y++) {
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
    int8_t destX = 0, maxY = 0;
    for (int8_t srcX = 0; srcX < field.width; srcX++) {
        uint32_t lastValue = field.objects[srcX];
        uint32_t value = closeFieldInColumn(srcX, maxY);
        if (value) {
            field.objects[destX] = value;
            destX++;
            if (value != lastValue) isFalled = true;
        }
        if (srcX == field.cursorX) field.cursorX = (destX > 0) ? destX - 1 : 0;
    }
    if (destX < field.width) {
        memset(field.objects + destX, 0, (field.width - destX) * sizeof(*field.objects));
        field.width = destX;
        isFalled = true;
    }
    field.height = maxY;
    if (field.cursorY >= maxY) field.cursorY = maxY - 1;
    dprint(F("Field width="));
    dprintln(field.width);
    dprint(F("Field height="));
    dprintln(field.height);
    return isFalled;
}

static uint32_t closeFieldInColumn(int8_t x, int8_t &maxY)
{
    if (!field.objects[x]) return 0;
    int8_t destY = 0;
    uint32_t value = 0;
    for (int8_t srcY = 0; srcY < field.height; srcY++) {
        uint8_t object = getField(x, srcY);
        if (object) {
            value |= (uint32_t) object << destY * 3;
            destY++;
        }
    }
    if (maxY < destY) maxY = destY;
    return value;
}

static bool judgeGameOver(void)
{
    for (int8_t x = 0; x < field.width; x++) {
        uint8_t lastObject = 0;
        for (int8_t y = 0; y < field.height; y++) {
            uint8_t object = getField(x, y);
            if (object) {
                if (object == lastObject || x < field.width - 1 && object == getField(x + 1, y)) {
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

static void onConfirmRetry(void)
{
    int16_t y = getMenuItemPos() * 6 + 22;
    setConfirmMenu(y, onRetry, onContinue);
    dprintln(F("Menu: confirm retry"));
}

static void onRetry(void)
{
    dprintln(F("Menu: retry"));
    initGame();
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

static void drawField(bool isFullUpdate)
{
    if (!isFullUpdate && linkedCount <= 1) return;
    int16_t offsetX = getOffsetX();
    uint8_t blinkFrac = counter & 7;
    for (int8_t x = 0; x < field.width; x++) {
        for (int8_t y = 0; y < field.height; y++) {
            uint8_t object = getField(x, y);
            if (object) {
                bool isLinked = getLinkedFlag(x, y);
                int16_t dx = x * GRID_SIZE + offsetX;
                int16_t dy = 58 - y * GRID_SIZE;
                if (isFullUpdate || isLinked && ((x + y) & 7) == blinkFrac) {
                    drawObject(dx, dy, object - 1);
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
    int16_t dy = 57 - y * GRID_SIZE;
    uint8_t color = (imgId == IMG_CURSOR_ID_CLEAR) ? BLACK : WHITE;
    arduboy.drawBitmap(dx, dy, imgCursor[imgId], IMG_CURSOR_W, IMG_CURSOR_H, color);
}

static void drawEraser(void)
{
    int16_t offsetX = getOffsetX() - 1;
    for (int8_t x = 0; x < field.width; x++) {
        for (int8_t y = 0; y < field.height; y++) {
            if (getLinkedFlag(x, y)) {
                int16_t dx = x * GRID_SIZE + offsetX;
                int16_t dy = 57 - y * GRID_SIZE;
                int8_t anim = counter - abs(field.cursorX - x) - abs(field.cursorY - y);
                if (anim >= 0 && anim < ERASING_ANIM * 2) {
                    uint8_t color = (anim < ERASING_ANIM) ? WHITE : BLACK;
                    if (color == BLACK) anim -= ERASING_ANIM;
                    arduboy.drawBitmap(dx, dy, imgEraser[anim], IMG_ERASER_W, IMG_ERASER_H, color);
                }
            }
        }
    }
}

static void drawResult(bool isFullUpdate)
{
    if (counter > FPS * 2 && arduboy.buttonPressed(B_BUTTON)) return;

    if (isPerfect && counter < FPS  - RESULT_ANIM) {
        arduboy.fillCircle(WIDTH / 2, HEIGHT / 2, counter * 5 + 2, WHITE);
    }
    int16_t dy = (isPerfect) ? 14 : 0;
    if (counter >= FPS  - RESULT_ANIM && (isFullUpdate || counter < FPS)) {
        drawResultLabel(dy, (counter < FPS) ? FPS - 1 - counter : 0);
    }
    if (counter >= FPS * 2 - RESULT_ANIM && (isFullUpdate || counter < FPS * 2)) {
        drawResultScore(dy + 10, (counter < FPS * 2) ? FPS * 2 - 1 - counter : 0);
    }
}

static void drawResultLabel(int16_t dy, uint8_t anim)
{
    arduboy.fillRect2(0, dy, WIDTH, 10, isPerfect);
    int16_t dx = anim * anim / 18;
    if (isPerfect) {
        dx += (WIDTH - IMG_PERFECT_W) / 2;
        arduboy.drawBitmap(dx, dy, imgPerfect, IMG_PERFECT_W, IMG_PERFECT_H, BLACK);
    } else {
        dx += (WIDTH - IMG_OVER_W) / 2;
        arduboy.drawBitmap(dx, dy, imgOver, IMG_OVER_W, IMG_OVER_H, WHITE);
    }
}

static void drawResultScore(int16_t dy, uint8_t anim)
{
    uint8_t digits = 0;
    for (uint16_t tmp = field.score; tmp > 0; tmp /= 10) {
        digits++;
    }
    arduboy.fillRect2(0, dy, WIDTH, (isHiscore) ? 24 : 18, isPerfect);
    arduboy.setTextColor(!isPerfect, !isPerfect);
    int16_t dx = anim * anim / 18;
    if (isHiscore) arduboy.printEx(dx + 31, dy + 18, F("NEW RECORD!"));
    int16_t dw = 30 + IMG_DIGIT_W * digits;
    dx += WIDTH / 2 - dw / 2;
    arduboy.printEx(dx, dy + 9, F("SCORE"));
    arduboy.setTextColor(WHITE, BLACK);
    dx += dw;
    for (uint16_t tmp = field.score; tmp > 0; tmp /= 10) {
        dx -= IMG_DIGIT_W;
        arduboy.drawBitmap(dx, dy, imgDigit[tmp % 10], IMG_DIGIT_W, IMG_DIGIT_H, !isPerfect);
    }
}
