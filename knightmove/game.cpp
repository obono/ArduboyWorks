#include "common.h"
#include "data.h"

/*  Defines  */

#define FIELD_W         8
#define FIELD_H         4

#define FLOOR_DEFAULT   3
#define FLOOR_OPEN      0

#define KNIGHT_D_MAX    8
#define KNIGHT_R_FAST   32
#define KNIGHT_R_MAX    384
#define KNIGHT_R_HALF   (KNIGHT_R_MAX / 2)
#define KNIGHT_JUMP     16

#define SCORE_MAX       9999999UL
#define LEVEL_MAX       20
#define HEART_LIFE_MAX  6

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
static void     steerKnight(int8_t vd);
static void     placeNewHeart(void);
static uint32_t calcPoint(int8_t n);

static void     onContinue(void);
static void     onConfirmRetry(void);
static void     onRetry(void);
static void     onConfirmQuit(void);
static void     onQuit(void);

static void     drawField(void);
static void     drawHeart(void);
static void     drawKnight(void);
static void     drawCursor(void);
static void     drawStatus(void);

/*  Local Functions (macros)  */

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))

/*  Local Variables  */

PROGMEM static const int8_t velocityTable[KNIGHT_D_MAX * 2] = {
    1, -2,  2, -1,  2, 1,   1, 2,   -1, 2,  -2, 1,  -2, -1, -1, -2
};

static STATE_T  state;
static int8_t   field[FIELD_H][FIELD_W];
static uint32_t score;
static uint16_t holes, knightR;
static int8_t   knightX, knightY, knightVx, knightVy, knightD;
static int8_t   heartX, heartY, heartNorm, heartLife;
static uint8_t  level, floors, continuous, counter;
static bool     isFast, isMenu;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    arduboy.playScore2(soundStart, SND_PRIO_START);
    lastScore = 0;
    initGameParams();
    counter = 0;
    state = STATE_START;
    isMenu = false;
    isInvalid = true;
}

MODE_T updateGame(void)
{
    counter++;
    if (state == STATE_MOVING || state == STATE_LEVEL_UP) {
        record.playFrames++;
        isRecordDirty = true;
    }
    handleDPad();
    if (isMenu) {
        handleMenu();
    } else {
        switch (state) {
        case STATE_START:
            handleStart();
            break;
        case STATE_MOVING:
            handleMoving();
            break;
        case STATE_LEVEL_UP:
            handleLevelUp();
            break;
        case STATE_FALLING:
            handleFalling();
            break;
        case STATE_OVER:
            handleOver();
            break;
        default:
            break;
        }
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (isMenu) {
        drawMenuItems(isInvalid);
        isInvalid = false;
        return;
    }
    if (isInvalid) {
        arduboy.clear();
        drawField();
        drawHeart();
        drawStatus();
        drawKnight();
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void initGameParams(void)
{
    score = 0;
    holes = 0;
    level = 1;
    continuous = 0;
    for (uint8_t y = 0; y < FIELD_H; y++) {
        for (uint8_t x = 0; x < FIELD_W; x++) {
            field[y][x] = FLOOR_DEFAULT;
        }
    }
    floors = FIELD_H * FIELD_W;
    knightX = random(FIELD_W);
    knightY = random(FIELD_H);
    knightD = random(KNIGHT_D_MAX);
    knightR = 0;
    steerKnight(1);
    heartX = -1;
    heartNorm = (isBMode) ? 3 : 1; // TODO
    isFast = false;
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
        knightR += KNIGHT_R_FAST;
        score++;
    } else {
        knightR += level;
    }
    if (knightR >= KNIGHT_R_MAX) {
        knightX += knightVx;
        knightY += knightVy;
        knightR = 0;
        isFast = false;
        int8_t floor = field[knightY][knightX] - 1;
        if (floor < FLOOR_OPEN) {
            arduboy.playScore2(soundOver, SND_PRIO_OVER);
            enterScore(score, isBMode);
            writeRecord();
            counter = 0;
            state = STATE_FALLING;
        } else {
            field[knightY][knightX] = floor;
            if (floor == FLOOR_OPEN) {
                continuous++;
                score += calcPoint(continuous);
                uint8_t idx = (continuous - 1) % 7;
                arduboy.playScore2((continuous <= 7) ? soundPointLow[idx] : soundPointHigh[idx],
                        SND_PRIO_BOUND);
                if (score > SCORE_MAX) score = SCORE_MAX;
                holes++;
                floors--;
            } else {
                arduboy.playScore2(soundBound, SND_PRIO_MISC);
                continuous = 0;
            }
            if (knightX == heartX && knightY == heartY) {
                arduboy.playScore2(soundHeart, SND_PRIO_EVENT);
                if (--heartNorm == 0) {
                    counter = 0;
                    heartX = 0;
                    heartY = 0;
                    state = STATE_LEVEL_UP;
                } else {
                    placeNewHeart();
                }
            } else {
                if (isBMode && --heartLife == 0) {
                    placeNewHeart();
                }
            }
        }
        if (state == STATE_MOVING) steerKnight(KNIGHT_D_MAX / 2);
    }
    isInvalid = true;
#ifdef DEBUG
    if (dbgRecvChar == 'z') {
    }
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
                continuous = 0;
                heartNorm = (isBMode) ? 3 : 1; // TODO
                steerKnight(KNIGHT_D_MAX / 2);
                placeNewHeart();
                state = STATE_MOVING;
            }
        }
        arduboy.playScore2(soundRepair[random(5)], SND_PRIO_MISC);
        counter = 0;
    }
    isInvalid = true;
}

static void handleFalling(void)
{
    if (counter >= FPS * 2) {
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

static uint32_t calcPoint(int8_t n)
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

static void steerKnight(int8_t vd)
{
    do {
        knightD = circulate(knightD, vd, 8);
        knightVx = pgm_read_byte(velocityTable + knightD * 2);
        knightVy = pgm_read_byte(velocityTable + knightD * 2 + 1);
    } while (knightX + knightVx < 0 || knightX + knightVx >= FIELD_W ||
             knightY + knightVy < 0 || knightY + knightVy >= FIELD_H);
}

static void placeNewHeart(void)
{
    heartLife = HEART_LIFE_MAX;
    if (floors < 1) {
        heartX = -1;
        return;
    }
    do {
        heartX = random(FIELD_W);
        heartY = random(FIELD_H);
    } while (field[heartY][heartX] == FLOOR_OPEN || heartX == knightX && heartY == knightY);
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
    isMenu = false;
    state = STATE_LEAVE;
    dprintln(F("Menu: quit"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawField(void)
{
    for (uint8_t y = 0; y < FIELD_H; y++) {
        for (uint8_t x = 0; x < FIELD_W; x++) {
            arduboy.drawBitmap(x * IMG_FLOOR_W + 16, y * IMG_FLOOR_H + 15,
                imgFloor[field[y][x]], IMG_FLOOR_W, IMG_FLOOR_H, WHITE);
        }
    }
    arduboy.drawRect2(14, 13, FIELD_W * IMG_FLOOR_W + 3, FIELD_H * IMG_FLOOR_H + 3, WHITE);
}

static void drawHeart(void)
{
    if (heartX < 0) return;
    if (state == STATE_MOVING && heartLife == 1 && (counter & 4)) return;
    int16_t dx = heartX * IMG_FLOOR_W + 16;
    int16_t dy = heartY * IMG_FLOOR_H + 15;
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
    int16_t dx = knightX * IMG_FLOOR_W + (int32_t)knightVx * knightR * IMG_FLOOR_W / KNIGHT_R_MAX;
    int16_t dy = knightY * IMG_FLOOR_H + (int32_t)knightVy * knightR * IMG_FLOOR_H / KNIGHT_R_MAX;
    int16_t h;
    uint8_t idx = knightD * 2 + bitRead(knightR, 5);
    if (state == STATE_START) {
        h = (FPS * 2 - counter) / 2;
        idx = 16 + bitRead(counter, 4);
    } else if (state == STATE_FALLING) {
        h = -counter;
    } else {
        h = abs(KNIGHT_R_HALF - knightR);
        h = KNIGHT_JUMP - (int32_t)h * h * KNIGHT_JUMP / KNIGHT_R_HALF / KNIGHT_R_HALF;
    }
    if (state != STATE_FALLING) {
        arduboy.drawBitmap(dx + 18, dy + 18, imgShadow, IMG_SHADOW_W, IMG_SHADOW_H, BLACK);
        if (state == STATE_MOVING) drawCursor();
        arduboy.drawBitmap(dx + 14, dy - h + 6, imgKnightMask[idx], IMG_KNIGHT_W, IMG_KNIGHT_H, BLACK);
    }
    arduboy.drawBitmap(dx + 14, dy - h + 6, imgKnight[idx], IMG_KNIGHT_W, IMG_KNIGHT_H, WHITE);
}

static void drawCursor(void)
{
    int16_t dx = (knightX + knightVx) * IMG_FLOOR_W + 14;
    int16_t dy = (knightY + knightVy) * IMG_FLOOR_H + 13;
    arduboy.drawBitmap(dx, dy, imgCursorMask, IMG_CURSOR_W, IMG_CURSOR_H, BLACK);
    arduboy.drawBitmap(dx, dy, imgCursor, IMG_CURSOR_W, IMG_CURSOR_H, WHITE);
}

static void drawStatus(void)
{
    arduboy.printEx(10, 0, F("SCORE"));
    drawNumberR(46, 6, score);
    arduboy.printEx(52, 0, F("LEVEL"));
    drawNumberR(76, 6, level);
    arduboy.printEx(88, 0, F("HOLES"));
    drawNumberR(112, 6, holes);
    for (uint8_t i = 0; i < heartNorm; i++) {
        arduboy.printEx(0, 58 - i * 6, F("#")); // TODO
    }
#if 1 //def DEBUG
    drawNumber(122, 58, state);
#endif
}
