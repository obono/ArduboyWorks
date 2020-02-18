#include "common.h"
#include "data.h"

/*  Defines  */

#define FIELD_W         8
#define FIELD_H         4
#define FLOOR_W         12
#define FLOOR_H         12

#define FLOOR_DEFAULT   3
#define FLOOR_OPEN      0

#define KNIGHT_DIR_MAX  8
#define KNIGHT_RAD_FAST 64
#define KNIGHT_RAD_MAX  768
#define KNIGHT_RAD_HALF (KNIGHT_RAD_MAX / 2)
#define KNIGHT_JUMP     16
#define KNIGHT_WAIT_MAX 6

#define SCORE_MAX       9999999UL
#define LEVEL_MAX       30
#define HEART_LIFE_MAX  6
#define POINT_LIFE_MAX  (FPS / 2)
#define EVAL_LIFE_MAX   (FPS * 2)

#define CONGRATS_1ST    15
#define CONGRATS_2ND    22
#define PETIT_H_RANGE   60

enum STATE_T : uint8_t {
    STATE_START = 0,
    STATE_MOVING,
    STATE_LEVEL_UP,
    STATE_FALLING,
    STATE_OVER,
    STATE_LEAVE,
};

/*  Typedefs  */

/*  Local Functions  */

static void     initGameParams(void);
static void     handleStart(void);
static void     handleMoving(void);
static void     handleLevelUp(void);
static void     handleFalling(void);
static void     handleOver(void);
static void     openMenu(void);
static void     steerKnight(int8_t vd);
static void     setHeartParams(int8_t n, bool isB);
static void     placeNewHeart(void);
static uint32_t getPoint(int8_t n);
static void     evaluate(int8_t n);
static void     setCongratsParams(bool isMulti);
static void     forwardCongrats(void);
static void     sparkleLed(uint8_t idx);

static void     onContinue(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawField(void);
static void     drawRollingFloor(void);
static void     drawHeart(void);
static void     drawKnight(void);
static void     drawCursor(void);
static void     drawCongrats(void);
static void     drawStatus(void);
static void     drawLargeLabel(void);
static void     drawLargeLabelBitmap(int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h);

/*  Local Functions (macros)  */

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))
#define callStateHandler(n) ((void (*)(void)) pgm_read_ptr(handlerFuncTable + n))()

/*  Local Variables  */

PROGMEM static const int8_t velocityTable[KNIGHT_DIR_MAX * 2] = {
    1, -2,  2, -1,  2, 1,   1, 2,   -1, 2,  -2, 1,  -2, -1, -1, -2
};

PROGMEM static void(*const handlerFuncTable[])(void) = {
    handleStart, handleMoving, handleLevelUp, handleFalling, handleOver
};

static STATE_T  state;
static int8_t   field[FIELD_H][FIELD_W];
static uint32_t score, point;
static uint16_t holes, knightRad, heartLife, heartLifeMax;
static int8_t   knightX, knightY, knightVx, knightVy, knightDir, knightWait;
static int8_t   heartX, heartY, heartNorm, heartNormMax;
static int8_t   petitX, petitY, petitH, petitCounter;
static uint8_t  level, floors, continuous, pointLife, evalLife, counter;
static uint8_t  ledRed, ledGreen, ledBlue, ledLevel;
static bool     isFast, isMenu, isHiscore;
static const __FlashStringHelper *evalLabel;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    arduboy.playScore2(soundStart, SND_PRIO_START);
    lastScore = 0;
    initGameParams();
    counter = 0;
    ledLevel = 0;
    isMenu = false;
    isHiscore = false;
    state = STATE_START;
    isInvalid = true;
}

MODE_T updateGame(void)
{
    handleDPad();
    if (isMenu) {
        handleMenu();
    } else {
        counter++;
        if (pointLife > 0) pointLife--;
        if (evalLife > 0) evalLife--;
        if (ledLevel > 0) ledLevel--;
        if (state == STATE_MOVING || state == STATE_LEVEL_UP) {
            record.playFrames++;
            isRecordDirty = true;
        }
        callStateHandler(state);
        if (petitCounter > 0) forwardCongrats();
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (isInvalid) {
        arduboy.clear();
        drawField();
        drawStatus();
        drawCongrats();
        drawKnight();
        drawLargeLabel();
    }
    if (isMenu) drawMenuItems(isInvalid);
    if (isHiscore && state == STATE_OVER) {
        arduboy.fillRect2(27, 50, 73, 7, BLACK);
        if (bitRead(counter, 3)) arduboy.printEx(34, 51, F("NEW RECORD"));
    }
    isInvalid = false;
    arduboy.setRGBled(ledRed * ledLevel, ledGreen * ledLevel, ledBlue * ledLevel);
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void initGameParams(void)
{
    score = 0;
    holes = 0;
    level = 0;
    continuous = 0;
    for (uint8_t y = 0; y < FIELD_H; y++) {
        for (uint8_t x = 0; x < FIELD_W; x++) {
            field[y][x] = FLOOR_DEFAULT;
        }
    }
    floors = FIELD_H * FIELD_W;
    knightX = random(FIELD_W);
    knightY = random(FIELD_H);
    knightDir = random(KNIGHT_DIR_MAX);
    knightRad = 0;
    knightWait = 0;
    steerKnight(1);
    heartX = -1;
    heartLife = 0;
    setHeartParams(level, isBMode);
    petitCounter = 0;
    isFast = false;
    pointLife = 0;
    evalLife = 0;
}

static void handleStart(void)
{
    if (counter >= FPS * 2) {
        record.playCount++;
        isRecordDirty = true;
        writeRecord();
        arduboy.playScore2(soundBound, SND_PRIO_BOUND);
        field[knightY][knightX]--;
        placeNewHeart();
        state = STATE_MOVING;
    }
    if (arduboy.buttonDown(A_BUTTON)) openMenu();
    isInvalid = true;
}

static void handleMoving(void)
{
    if (!isFast) {
        if (padX != 0) {
            playSoundTick();
            steerKnight(padX);
        }
        if (arduboy.buttonDown(B_BUTTON)) {
        arduboy.playScore2(soundFast, SND_PRIO_MISC);
            isFast = true;
        }
    }
    if (isFast) {
        knightRad += KNIGHT_RAD_FAST;
        score++;
    } else {
        if (knightWait > 0) {
            knightWait--;
        } else {
            knightRad += level + 2;
        }
    }
    if (isBMode && heartLife > 0) heartLife--;
    if (continuous >= 8 && ledLevel < LED_LEVEL_POINT_BLK) {
        ledLevel = bitRead(counter, 1) * LED_LEVEL_POINT_BLK;
    }

    if (knightRad >= KNIGHT_RAD_MAX) {
        knightX += knightVx;
        knightY += knightVy;
        knightRad = 0;
        isFast = false;
        int8_t floor = field[knightY][knightX] - 1;
        if (floor < FLOOR_OPEN) {
            arduboy.playScore2(soundOver, SND_PRIO_OVER);
            isHiscore = enterScore(score, isBMode);
            writeRecord();
            evalLife = 0;
            counter = 0;
            state = STATE_FALLING;
            dprintln(F("Game over"));
        } else {
            field[knightY][knightX] = floor;
            knightWait = KNIGHT_WAIT_MAX;
            if (floor == FLOOR_OPEN) {
                continuous++;
                if (continuous == CONGRATS_1ST) setCongratsParams(false);
                if (continuous == CONGRATS_2ND) setCongratsParams(true);
                point = getPoint(continuous);
                pointLife = POINT_LIFE_MAX;
                score += point;
                if (score > SCORE_MAX) score = SCORE_MAX;
                holes++;
                floors--;
                uint8_t idx = (continuous - 1) % 7;
                arduboy.playScore2((continuous <= 7) ? soundPointLow[idx] : soundPointHigh[idx],
                        SND_PRIO_BOUND);
                sparkleLed(idx);
                dprint(point);
                dprintln(F(" Points"));
            } else {
                evaluate(continuous);
                continuous = 0;
                if (evalLife == EVAL_LIFE_MAX) {
                    arduboy.playScore2(soundEvaluate, SND_PRIO_EVENT);
                } else {
                    arduboy.playScore2(soundBound, SND_PRIO_MISC);
                }
            }
            if (knightX == heartX && knightY == heartY) {
                arduboy.playScore2(soundHeart, SND_PRIO_EVENT);
                dprintln(F("Get heart"));
                if (--heartNorm == 0) {
                    evaluate(continuous);
                    sparkleLed(LED_IDX_HEART);
                    continuous = 0;
                    heartX = 0;
                    heartY = 0;
                    heartLife = 0;
                    counter = 0;
                    state = STATE_LEVEL_UP;
                } else {
                    if (continuous == 0) sparkleLed(LED_IDX_HEART);
                    placeNewHeart();
                }
            } else {
                if (isBMode && heartLife == 0) {
                    placeNewHeart();
                }
            }
        }
        if (state == STATE_MOVING) steerKnight(KNIGHT_DIR_MAX / 2);
    }
    if (state != STATE_FALLING && arduboy.buttonDown(A_BUTTON)) openMenu();
    isInvalid = true;
#ifdef DEBUG
    if (dbgRecvChar == 'z') {
        int8_t x = knightX + knightVx, y = knightY + knightVy;
        if (field[y][x] == FLOOR_OPEN) floors++;
        field[y][x] = 1;
    }
    if (dbgRecvChar == 'x') {
        heartX = knightX + knightVx;
        heartY = knightY + knightVy;
        if (field[heartY][heartX] == FLOOR_OPEN) floors++;
        field[heartY][heartX] = FLOOR_DEFAULT;
    }
    if (dbgRecvChar == 'q') setCongratsParams(false);
    if (dbgRecvChar == 'w') setCongratsParams(true);

#endif
}

static void handleLevelUp(void)
{
    if (counter == 8) {
        if (field[heartY][heartX] == FLOOR_OPEN) {
            field[heartY][heartX] = FLOOR_DEFAULT;
            floors++;
        }
        if (++heartY >= FIELD_H) {
            heartY = 0;
            if (++heartX >= FIELD_W) {
                arduboy.playScore2(soundLevelUp, SND_PRIO_EVENT);
                if (level < LEVEL_MAX) level++;
                steerKnight(KNIGHT_DIR_MAX / 2);
                knightWait = 0;
                setHeartParams(level, isBMode);
                placeNewHeart();
                state = STATE_MOVING;
                dprint(F("Level "));
                dprintln(level);
            }
        }
        arduboy.playScore2(soundRepair[random(5)], SND_PRIO_MISC);
        counter = 0;
    }
    if (arduboy.buttonDown(A_BUTTON)) openMenu();
    isInvalid = true;
}

static void handleFalling(void)
{
    if (counter >= FPS) {
        state = STATE_OVER;
    }
    isInvalid = true;
}

static void handleOver(void)
{
    if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        state = STATE_LEAVE;
        isInvalid = true;
    } else if (arduboy.buttonDown(B_BUTTON)) {
        initGame();
    }
}

static void openMenu(void)
{
    playSoundClick();
    writeRecord();
    clearMenuItems();
    addMenuItem(F("CONTINUE"), onContinue);
    if (state != STATE_START) addMenuItem(F("RESTART GAME"), onConfirmRetry);
    addMenuItem(F("BACK TO TITLE"), (state == STATE_START) ? onQuit : onConfirmQuit);
    int8_t n = getMenuItemCount();
    setMenuCoords(19, 39 - n * 3, 89, n * 6 - 1, true, true);
    setMenuItemPos(0);
    ledLevel = 0;
    isMenu = true;
    dprintln(F("Open menu"));
}

static void steerKnight(int8_t vd)
{
    do {
        knightDir = circulate(knightDir, vd, 8);
        knightVx = pgm_read_byte(velocityTable + knightDir * 2);
        knightVy = pgm_read_byte(velocityTable + knightDir * 2 + 1);
    } while (knightX + knightVx < 0 || knightX + knightVx >= FIELD_W ||
             knightY + knightVy < 0 || knightY + knightVy >= FIELD_H);
}

static void setHeartParams(int8_t n, bool isB)
{
    if (isB) {
        heartNormMax = 3 + (n >= 5) + (n >= 10) + (n >= 20);
        heartLifeMax = (KNIGHT_RAD_MAX / (n + 2) + KNIGHT_WAIT_MAX) * heartNormMax;
    } else {
        heartNormMax = 1;
        heartLifeMax = 1;
    }
    heartNorm = heartNormMax;
}

static void placeNewHeart(void)
{
    if (floors < 1) {
        heartX = -1;
        heartLife = 0;
        dprintln(F("No place for heart..."));
        return;
    }
    do {
        heartX = random(FIELD_W);
        heartY = random(FIELD_H);
    } while (field[heartY][heartX] == FLOOR_OPEN || heartX == knightX && heartY == knightY);
    heartLife = heartLifeMax;
}

static uint32_t getPoint(int8_t n)
{
    if (n <= 7) {
        return 100 * n;
    } else if (n <= 14) {
        return 1000 + 200 * (n - 8);
    } else if (n <= 21) {
        return 3000 + 500 * (n - 15);
    } else if (n <= 28) {
        return 10000 + 2000 * (n - 22);
    } else if (n <= 31) {
        return 30000UL + 10000UL * (n - 29);
    } else {
        return 100000UL;
    }
}

static void evaluate(int8_t n)
{
#ifdef DEBUG
    if (n > 0) {
        dprint(F("Continuous="));
        dprintln(n);
    }
#endif
    if (n <= 7) {
        return;
    } else if (n <= 14) {
        evalLabel = F("COOL");
    } else if (n <= 21) {
        evalLabel = F("VERY GOOD");
    } else if (n <= 28) {
        evalLabel = F("WONDERFUL");
    } else if (n <= 31) {
        evalLabel = F("EXCELLENT!");
    } else {
        evalLabel = F("PERFECT!!");
    }
    evalLife = EVAL_LIFE_MAX;
}

static void setCongratsParams(bool isMulti)
{
    petitX = knightX;
    petitY = knightY;
    petitH = PETIT_H_RANGE;
    petitCounter = (isMulti) ? 3 : 1;
}

static void forwardCongrats(void)
{
    petitH -= (petitCounter == 2) ? 1 : 2;
    if (petitH <= -PETIT_H_RANGE) {
        petitH = PETIT_H_RANGE;
        petitCounter--;
    }
}

static void sparkleLed(uint8_t idx)
{
    uint8_t ledValue = pgm_read_byte(ledValues + idx);
    ledRed = ledValue / 25;
    ledGreen = ledValue / 5 % 5;
    ledBlue = ledValue % 5;
    ledLevel = (idx == LED_IDX_HEART) ? LED_LEVEL_HEART : LED_LEVEL_POINT;
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onContinue(void)
{
    playSoundClick();
    isMenu = false;
    isInvalid = true;
    dprintln(F("Menu: continue"));
}

static void onConfirmRetry(void)
{
    int16_t y = getMenuItemPos() * 6 + 32;
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
    int16_t y = getMenuItemPos() * 6 + 32;
    setConfirmMenu(y, onQuit, onContinue);
    dprintln(F("Menu: confirm quit"));
}

static void onQuit(void)
{
    playSoundClick();
    writeRecord();
    isMenu = false;
    state = STATE_LEAVE;
    dprintln(F("Menu: quit"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawField(void)
{
    if (!isMenu) {
        for (uint8_t y = 0; y < FIELD_H; y++) {
            for (uint8_t x = 0; x < FIELD_W; x++) {
                arduboy.drawBitmap(x * FLOOR_W + 16, y * FLOOR_H + 15,
                    imgFloor[field[y][x]], IMG_FLOOR_W, IMG_FLOOR_H, WHITE);
            }
        }
        if (state == STATE_MOVING) drawRollingFloor();
        drawHeart();
    }

    if (isMenu || state != STATE_MOVING || continuous <= 7 || bitRead(counter, 1)) {
        arduboy.drawRect2(14, 13, FIELD_W * FLOOR_W + 3, FIELD_H * FLOOR_H + 3, WHITE);
    }
}

static void drawRollingFloor(void)
{
    int16_t dx = knightX * FLOOR_W + 16;
    int16_t dy = knightY * FLOOR_H + 15;
    int16_t g = (1.0 - sin((counter & 15) * PI / 16.0)) * (IMG_FLOOR_H / 2.0);
    arduboy.fillRect2(dx, dy,                   IMG_FLOOR_W, g, BLACK);
    arduboy.fillRect2(dx, dy + IMG_FLOOR_H - g, IMG_FLOOR_W, g, BLACK);
    if (field[knightY][knightX] == FLOOR_OPEN) {
        int16_t dx2 = dx + IMG_FLOOR_W - 1;
        int16_t dy2 = dy + g;
        arduboy.drawPixel(dx,  dy2, WHITE);
        arduboy.drawPixel(dx2, dy2, WHITE);
        dy2 = dy + IMG_FLOOR_H - g - 1;
        arduboy.drawPixel(dx,  dy2, WHITE);
        arduboy.drawPixel(dx2, dy2, WHITE);
    }
}

static void drawHeart(void)
{
    if (heartX < 0) return;
    if (state == STATE_MOVING && heartLife == 0 && (counter & 4)) return;
    int16_t dx = heartX * FLOOR_W + 16;
    int16_t dy = heartY * FLOOR_H + 15;
    if (state == STATE_LEVEL_UP) {
        dx += random(-1, 2);
        dy += random(-1, 2);
    }
    arduboy.drawBitmap(dx, dy, imgHeartMask, IMG_HEART_W, IMG_HEART_H, BLACK);
    arduboy.drawBitmap(dx, dy, imgHeart, IMG_HEART_W, IMG_HEART_H, WHITE);
}

static void drawKnight(void)
{
    if (state == STATE_OVER) return;
    int16_t dx = knightX * FLOOR_W + (int32_t)knightVx * knightRad * FLOOR_W / KNIGHT_RAD_MAX;
    int16_t dy = knightY * FLOOR_H + (int32_t)knightVy * knightRad * FLOOR_H / KNIGHT_RAD_MAX;
    uint8_t idx = knightDir * 2 + bitRead(knightRad, 5);
    int16_t h;
    if (state == STATE_START) {
        idx = 16 + bitRead(counter, 4);
        h = (FPS * 2 - counter) / 2;
    } else if (state == STATE_FALLING) {
        h = -counter;
    } else {
        h = abs(KNIGHT_RAD_HALF - knightRad);
        h = KNIGHT_JUMP - (int32_t)h * h * KNIGHT_JUMP / KNIGHT_RAD_HALF / KNIGHT_RAD_HALF;
    }
    if (state != STATE_FALLING) {
        if (bitRead(counter, 0)) {
            arduboy.drawBitmap(dx + 18, dy + 19, imgShadow, IMG_SHADOW_W, IMG_SHADOW_H, BLACK);
        }
        if (state == STATE_MOVING) drawCursor();
        arduboy.drawBitmap(dx + 14, dy - h + 7, imgKnightMask[idx],
                IMG_KNIGHT_W, IMG_KNIGHT_H, BLACK);
    }
    arduboy.drawBitmap(dx + 14, dy - h + 7, imgKnight[idx], IMG_KNIGHT_W, IMG_KNIGHT_H, WHITE);
}

static void drawCursor(void)
{
    int16_t dx = (knightX + knightVx) * FLOOR_W + 14;
    int16_t dy = (knightY + knightVy) * FLOOR_H + 13;
    arduboy.drawBitmap(dx, dy, imgCursorMask, IMG_CURSOR_W, IMG_CURSOR_H, BLACK);
    arduboy.drawBitmap(dx, dy, imgCursor, IMG_CURSOR_W, IMG_CURSOR_H, WHITE);
}

static void drawCongrats(void)
{
    if (petitCounter <= 0) return;
    int16_t dx = petitX * FLOOR_W + 17;
    if (petitCounter == 2) {
        int8_t v = counter & 15;
        dx += (v < 8) ? v - 4 : 12 - v;
    }
    int16_t dy = petitY * FLOOR_H + 17 + petitH;
    uint8_t idx = bitRead(counter, 1);
    if (petitH < 0) arduboy.drawBitmap(dx, dy, imgPetitMask[idx], IMG_PETIT_W, IMG_PETIT_H, BLACK);
    arduboy.drawBitmap(dx, dy, imgPetit[idx], IMG_PETIT_W, IMG_PETIT_H, WHITE);
}

static void drawStatus(void)
{
    arduboy.printEx(10, 0, F("SCORE"));
    if (pointLife > 0 && !isMenu) {
        if (bitRead(counter, 1)) {
            int8_t len = drawNumberR(46, 6, point);
            arduboy.printEx(46 - len * 6, 6, F("+"));
        }
    } else {
        drawNumberR(46, 6, score);
    }

    if (evalLife > 0 && !isMenu) {
        if (bitRead(counter, 2)) {
            int8_t len = strlen_P((const char *)evalLabel);
            arduboy.printEx(85 - len * 3, 3, evalLabel);
        }
    } else {
        arduboy.printEx(52, 0, F("LEVEL"));
        drawNumberR(76, 6, level);
        arduboy.printEx(88, 0, F("HOLES"));
        drawNumberR(112, 6, holes);
    }

    if (isBMode) {
        for (uint8_t i = 0; i < heartNormMax; i++) {
            arduboy.setCursor(6, 57 - i * 6);
            arduboy.print((char)('^' + (i < heartNorm)));
        }
        uint8_t h = (FIELD_H * FLOOR_H + 3) * heartLife / heartLifeMax;
        arduboy.fillRect2(117, 64 - h, 2, h, WHITE);
    }

#ifdef DEBUG
    drawNumber(122, 58, state);
#endif
}

static void drawLargeLabel()
{
    if (state == STATE_START) {
        int16_t y = 31;
        if (counter > FPS) y += (counter - FPS) / 2;
        drawLargeLabelBitmap(y, imgReady, IMG_READY_W, IMG_READY_H);
    }
    if (state == STATE_FALLING || state == STATE_OVER) {
        int16_t y = 31;
        y += (counter < FPS - 10) ? (FPS - 10) - counter : -bitRead(counter, 0);
        drawLargeLabelBitmap(y, imgOver, IMG_OVER_W, IMG_OVER_H);
    }
}

static void drawLargeLabelBitmap(int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h)
{
    int16_t x = (WIDTH - w) / 2;
    for (int8_t i = 0; i < 4; i++) {
        arduboy.drawBitmap(x + (i + 1) % 3 - 1, y + (i * 2 - 3) / 3, bitmap, w, h, BLACK);
    }
    arduboy.drawBitmap(x, y, bitmap, w, h, WHITE);
}
