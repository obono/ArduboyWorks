#include "common.h"
#include "image.h"

/*  Defines  */

enum STATE_T {
    STATE_INIT = 0,
    STATE_PLAYING,
    STATE_OVER,
    STATE_LEAVE,
};

enum OBJ_TYPE {
    OBJ_TYPE_FLOOR = 0,
    OBJ_TYPE_1,
    OBJ_TYPE_2,
    OBJ_TYPE_3,
    OBJ_TYPE_4,
    OBJ_TYPE_5,
    OBJ_TYPE_6,
    OBJ_TYPE_BLANK,
};

enum OBJ_MODE
{
    OBJ_MODE_APPEAR = 0,
    OBJ_MODE_NORMAL,
    OBJ_MODE_FIXED,
    OBJ_MODE_VANISH,
};

#define FIELD_W     7
#define FIELD_H     5
#define DEPTH_MAX   (FPS * 8)
#define DEPTH_PIXEL 4
#define LEVEL_MAX   99
#define CHAIN_MAX   999
#define SCORE_MAX   9999999

#define COUNT_DICE_INITIAL      15
#define FRAMES_3MINUTES         (FPS * 60 * 3)
#define VANISH_FLASH_FRAMES_MAX 6
#define SHOW_CHAIN_FRAMES_MAX   (FPS * 5)

/*  Typedefs  */

typedef struct {
    uint16_t    type : 3;   // 0:Floor / 1-6:Dice / 7:Blank
    uint16_t    rotate : 2; // 0-3
    uint16_t    mode : 2;   // 0:Appear / 1:Normal / 2:Fixed / 3:Vanish
    uint16_t    depth : 9;  // 0-320
    uint16_t    chain : 10; // 0-999
    uint16_t    dummy : 6;
} OBJ_T;

/*  Local Functions  */

static void initTrialField(void);
static void initPuzzleField(void);
static void updateField(void);
static void updateObject(OBJ_T *pObj);
static OBJ_T *pickFloorObject(void);
static void setDie(OBJ_T *pDie, OBJ_MODE mode);

static void moveCursor(void);
static void rotateDie(OBJ_T *pDie, int vx, int vy);
static void judgeVanish(int x, int y);
static bool judgeHappyOne(int x, int y);
static int  judgeLinkedDice(int x, int y, uint16_t type, uint16_t *pMaxChain);
static int  vanishHappyOne(void);
static void vanishLinkedDice(uint16_t chain);
static void vanishDie(OBJ_T *pObj, uint16_t chain);

static void setLevel(int level);
static bool isGameOver(void);
static void checkHiscore(void);

static void drawField(void);
static void drawObject(int x, int y, OBJ_T obj);
static void drawFloor(int16_t dx, int16_t dy);
static void drawDie(int16_t dx, int16_t dy, uint16_t type, uint16_t rotate, boolean isWhite);
static void drawObjectBitmap(int16_t x, int16_t y, int id, uint8_t c);
static void drawCursor(void);
static void drawStrings(void);
static void drawLargeLabel(void);

/*  Local Variables  */

PROGMEM static const uint8_t dieRotateMap[6 * 4 * 4] = {
    IMG_OBJ_DIE_2C, IMG_OBJ_DIE_3B, IMG_OBJ_DIE_4D, IMG_OBJ_DIE_5A, // 1A
    IMG_OBJ_DIE_3C, IMG_OBJ_DIE_5B, IMG_OBJ_DIE_2D, IMG_OBJ_DIE_4A, // 1B
    IMG_OBJ_DIE_5C, IMG_OBJ_DIE_4B, IMG_OBJ_DIE_3D, IMG_OBJ_DIE_2A, // 1C
    IMG_OBJ_DIE_4C, IMG_OBJ_DIE_2B, IMG_OBJ_DIE_5D, IMG_OBJ_DIE_3A, // 1D
    IMG_OBJ_DIE_1C, IMG_OBJ_DIE_4A, IMG_OBJ_DIE_3A, IMG_OBJ_DIE_6A, // 2A
    IMG_OBJ_DIE_4B, IMG_OBJ_DIE_6B, IMG_OBJ_DIE_1D, IMG_OBJ_DIE_3B, // 2B
    IMG_OBJ_DIE_6C, IMG_OBJ_DIE_3C, IMG_OBJ_DIE_4C, IMG_OBJ_DIE_1A, // 2C
    IMG_OBJ_DIE_3D, IMG_OBJ_DIE_1B, IMG_OBJ_DIE_6D, IMG_OBJ_DIE_4D, // 2D
    IMG_OBJ_DIE_1D, IMG_OBJ_DIE_2A, IMG_OBJ_DIE_5A, IMG_OBJ_DIE_6D, // 3A
    IMG_OBJ_DIE_2B, IMG_OBJ_DIE_6A, IMG_OBJ_DIE_1A, IMG_OBJ_DIE_5B, // 3B
    IMG_OBJ_DIE_6B, IMG_OBJ_DIE_5C, IMG_OBJ_DIE_2C, IMG_OBJ_DIE_1B, // 3C
    IMG_OBJ_DIE_5D, IMG_OBJ_DIE_1C, IMG_OBJ_DIE_6C, IMG_OBJ_DIE_2D, // 3D
    IMG_OBJ_DIE_1B, IMG_OBJ_DIE_5A, IMG_OBJ_DIE_2A, IMG_OBJ_DIE_6B, // 4A
    IMG_OBJ_DIE_5B, IMG_OBJ_DIE_6C, IMG_OBJ_DIE_1C, IMG_OBJ_DIE_2B, // 4B
    IMG_OBJ_DIE_6D, IMG_OBJ_DIE_2C, IMG_OBJ_DIE_5C, IMG_OBJ_DIE_1D, // 4C
    IMG_OBJ_DIE_2D, IMG_OBJ_DIE_1A, IMG_OBJ_DIE_6A, IMG_OBJ_DIE_5D, // 4D
    IMG_OBJ_DIE_1A, IMG_OBJ_DIE_3A, IMG_OBJ_DIE_4A, IMG_OBJ_DIE_6C, // 5A
    IMG_OBJ_DIE_3B, IMG_OBJ_DIE_6D, IMG_OBJ_DIE_1B, IMG_OBJ_DIE_4B, // 5B
    IMG_OBJ_DIE_6A, IMG_OBJ_DIE_4C, IMG_OBJ_DIE_3C, IMG_OBJ_DIE_1C, // 5C
    IMG_OBJ_DIE_4D, IMG_OBJ_DIE_1D, IMG_OBJ_DIE_6B, IMG_OBJ_DIE_3D, // 5D
    IMG_OBJ_DIE_2A, IMG_OBJ_DIE_4D, IMG_OBJ_DIE_3B, IMG_OBJ_DIE_5C, // 6A
    IMG_OBJ_DIE_4A, IMG_OBJ_DIE_5D, IMG_OBJ_DIE_2B, IMG_OBJ_DIE_3C, // 6B
    IMG_OBJ_DIE_5A, IMG_OBJ_DIE_3D, IMG_OBJ_DIE_4B, IMG_OBJ_DIE_2C, // 6C
    IMG_OBJ_DIE_3A, IMG_OBJ_DIE_2D, IMG_OBJ_DIE_5B, IMG_OBJ_DIE_4C, // 6D
};

PROGMEM static const uint8_t imgObjIdMap[6 * 2] = {
     IMG_OBJ_DIE_W_1,  IMG_OBJ_DIE_W_1,  // 1
     IMG_OBJ_DIE_W_2A, IMG_OBJ_DIE_W_2B, // 2
     IMG_OBJ_DIE_W_3A, IMG_OBJ_DIE_W_3B, // 3
     IMG_OBJ_DIE_W_4,  IMG_OBJ_DIE_W_4,  // 4
     IMG_OBJ_DIE_W_5,  IMG_OBJ_DIE_W_5,  // 5
     IMG_OBJ_DIE_W_6A, IMG_OBJ_DIE_W_6B, // 6
};

static STATE_T  state = STATE_INIT;
static OBJ_T    field[FIELD_H][FIELD_W];
static uint8_t  vanishFlg[FIELD_H];

static uint32_t score;
static uint32_t gameFrames;

static int16_t  elaspedFrames;
static uint16_t maxChain;
static uint16_t currentChain;

static int8_t   cursorX, cursorY;
static int8_t   level, norm;
static uint8_t  countDice, countValidDice;
static uint8_t  countDiceMin, countDiceUsual, countDiceMax;
static uint8_t  intervalDieUsual, intervalDieMax;
static uint8_t  vanishFlashFrames, showChainFrames;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    switch (gameMode) {
    case GAME_MODE_ENDLESS:
        initTrialField();
        setLevel(1);
        break;
    case GAME_MODE_LIMITED:
        initTrialField();
        countDiceMin = 15;
        countDiceUsual = 22;
        countDiceMax = 30;
        intervalDieUsual = FPS * 3;
        intervalDieMax = FPS * 5;
        break;
    case GAME_MODE_PUZZLE:
        initPuzzleField();
        break;
    }

    cursorX = (FIELD_W - 1) / 2;
    cursorY = (FIELD_H - 1) / 2;
    vanishFlashFrames = 0;
    elaspedFrames = 0;
    gameFrames = 0;
    maxChain = 0;
    showChainFrames = 0;
    score = 0;
    state = STATE_PLAYING;
    isInvalid = true;
    isRecordDirty = true;
}

MODE_T updateGame(void)
{
    if (state == STATE_PLAYING) {
        handleDPad();
        updateField();
        moveCursor();
        if (gameMode == GAME_MODE_ENDLESS) {
            if (level < LEVEL_MAX && norm <= 0) setLevel(level + 1);
        }
        if (vanishFlashFrames > 0) vanishFlashFrames--;
        if (showChainFrames > 0) showChainFrames--;
        gameFrames++;
        record.playFrames++;
        if (isGameOver()) {
            state = STATE_OVER;
            checkHiscore();
            writeRecord();
        }
        isInvalid = true;
    }

    if (arduboy.buttonDown(A_BUTTON)) { // TODO
        state = STATE_LEAVE;
        writeRecord();
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (isInvalid) {
        arduboy.clear();
        drawField();
        if (state == STATE_PLAYING) {
            drawStrings();
            drawCursor();
        }
        drawLargeLabel();
        isInvalid = false;
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void initTrialField(void)
{
    for (int y = 0; y < FIELD_H; y++) {
        for (int x = 0; x < FIELD_W; x++) {
            OBJ_T *pObj = &field[y][x];
            pObj->type = OBJ_TYPE_FLOOR;
            pObj->depth = DEPTH_MAX;
        }
    }
    countDice = countValidDice = 0;
    for (int i = 0; i < COUNT_DICE_INITIAL; i++) {
        setDie(pickFloorObject(), OBJ_MODE_NORMAL);
    }
    dprintln(F("Field initialized"));
}

static void initPuzzleField(void)
{
    // TODO
    countDice = countValidDice = 0;
    dprintln(F("Puzzle initialized"));
}

static void updateField(void)
{
    for (int y = 0; y < FIELD_H; y++) {
        for (int x = 0; x < FIELD_W; x++) {
            updateObject(&field[y][x]);
        }
    }

    if (gameMode != GAME_MODE_PUZZLE) {
        if (elaspedFrames < intervalDieMax) elaspedFrames++;
        if (countDice < countDiceMin ||
                countDice < countDiceUsual && elaspedFrames >= intervalDieUsual ||
                countDice < countDiceMax && elaspedFrames >= intervalDieMax) {
            setDie(pickFloorObject(), OBJ_MODE_APPEAR);
            elaspedFrames = min(elaspedFrames, 0);
        }
    }
}

static void updateObject(OBJ_T *pObj)
{
    if (pObj->type == OBJ_TYPE_FLOOR || pObj->type == OBJ_TYPE_BLANK) return;
    switch (pObj->mode) {
    case OBJ_MODE_APPEAR:
        pObj->depth -= 4;
        if (pObj->depth == 0) {
            pObj->mode = OBJ_MODE_NORMAL;
            countValidDice++;
        }
        break;
    case OBJ_MODE_VANISH:
        if (++pObj->depth >= DEPTH_MAX) {
            pObj->type = OBJ_TYPE_FLOOR;
            pObj->depth = DEPTH_MAX;
            countDice--;
        }
        break;
    case OBJ_MODE_NORMAL:
    case OBJ_MODE_FIXED:
        // do nothing
        break;
    }
}

static OBJ_T *pickFloorObject(void)
{
    OBJ_T *pObj;
    do {
        pObj = &field[random(FIELD_H)][random(FIELD_W)];
    } while (pObj->type != OBJ_TYPE_FLOOR);
    return pObj;
}

static void setDie(OBJ_T *pDie, OBJ_MODE mode)
{
    pDie->type = random(OBJ_TYPE_1, OBJ_TYPE_6 + 1);
    pDie->rotate = random(4);
    pDie->mode = mode;
    pDie->depth = (mode == OBJ_MODE_APPEAR) ? DEPTH_MAX : 0;
    pDie->chain = 0;
    countDice++;
    if (mode != OBJ_MODE_APPEAR) countValidDice++;
}

static void moveCursor(void)
{
    if (cursorX + padX < 0 || cursorX + padX >= FIELD_W) padX = 0;
    if (cursorY + padY < 0 || cursorY + padY >= FIELD_H) padY = 0;
    if (padX != 0 || padY != 0) {
        OBJ_T cursorObj = field[cursorY][cursorX];
        if (arduboy.buttonPressed(B_BUTTON) && cursorObj.type >= OBJ_TYPE_1 &&
                cursorObj.type <= OBJ_TYPE_6 && cursorObj.depth == 0) {
            if (padX != 0 && padY != 0) {
                if (gameFrames % 2) {
                    padX = 0;
                } else {
                    padY = 0;
                }
            }
            OBJ_T destObj = field[cursorY + padY][cursorX + padX];
            if (cursorObj.mode == OBJ_MODE_NORMAL && (
                    destObj.type == OBJ_TYPE_FLOOR ||
                    destObj.type <= OBJ_TYPE_6 && destObj.depth >= DEPTH_MAX / 2)) {
                if (destObj.type != OBJ_TYPE_FLOOR && destObj.mode == OBJ_MODE_VANISH) {
                    destObj.type = OBJ_TYPE_FLOOR;
                    destObj.depth = DEPTH_MAX;
                    countDice--;
                }
                field[cursorY][cursorX] = destObj;
                cursorX += padX;
                cursorY += padY;
                rotateDie(&cursorObj, padX, padY);
                field[cursorY][cursorX] = cursorObj;
                judgeVanish(cursorX, cursorY);
            }
        } else {
            cursorX += padX;
            cursorY += padY;
        }
    }
}

static void rotateDie(OBJ_T *pDie, int vx, int vy)
{
    int idx = (pDie->type - OBJ_TYPE_1) * 16 + pDie->rotate * 4 + (vy * 3 + vx + 3) / 2;
    int tmpId = pgm_read_byte(dieRotateMap + idx);
    pDie->type = tmpId / 4 + OBJ_TYPE_1;
    pDie->rotate = tmpId % 4;
}

static void judgeVanish(int x, int y)
{
    memset(vanishFlg, 0, FIELD_H);
    OBJ_T die = field[y][x];
    if (die.type == OBJ_TYPE_1) {
        if (judgeHappyOne(x, y)) {
            int link = vanishHappyOne();
            score += link;
            dprint(F("Happy one "));
            dprintln(link);
        }
    } else {
        uint16_t chain;
        int link = judgeLinkedDice(x, y, die.type, &chain);
        if (link >= die.type) {
            if (chain < CHAIN_MAX) chain++;
            vanishLinkedDice(chain);
            score += die.type * link * chain;
            maxChain = max(maxChain, chain);
            dprint(F("Valnish "));
            dprint(die.type * link);
            dprint(F(" x "));
            dprintln(chain);
        }
    }
    score = min(score, SCORE_MAX);
}

static bool judgeHappyOne(int x, int y)
{
    for (int dir = 1; dir < 8; dir += 2) {
        int fx = x + dir % 3 - 1;
        int fy = y + dir / 3 - 1;
        if (fx >= 0 && fx < FIELD_W && fy >= 0 && fy < FIELD_H) {
            OBJ_T obj = field[fy][fx];
            if (obj.type >= OBJ_TYPE_1 && obj.type <= OBJ_TYPE_6 && obj.mode == OBJ_MODE_VANISH) {
                return true;
            }
        }
    }
    return false;
}

static int judgeLinkedDice(int x, int y, uint16_t type, uint16_t *pChain)
{
    int link = 0, stackPos = 0;
    uint8_t stack[FIELD_W * FIELD_H + 1];
    uint8_t dir = 1;
    *pChain = 0;
    while (stackPos >= 0) {
        if (x < 0 || x >= FIELD_W || y < 0 || y >= FIELD_H ||
            (vanishFlg[y] & 1 << x) || field[y][x].type != type) {
            do {
                if (--stackPos < 0) {
                    goto bail;
                }
                dir = stack[stackPos];
                x -= dir % 3 - 1;
                y -= dir / 3 - 1;
            } while (dir == 7);
            dir += 2;
        } else {
            link++;
            *pChain = max(*pChain, field[y][x].chain);
            vanishFlg[y] |= 1 << x;
        }
        x += dir % 3 - 1;
        y += dir / 3 - 1;
        stack[stackPos++] = dir;
        dir = 1;
    }
bail:
    return link;
}

static int vanishHappyOne(void)
{
    int count = 0;
    for (int y = 0; y < FIELD_H; y++) {
        for (int x = 0; x < FIELD_W; x++) {
            OBJ_T *pObj = &field[y][x];
            if (pObj->type == OBJ_TYPE_1 &&
                    (pObj->mode == OBJ_MODE_NORMAL || pObj->mode == OBJ_MODE_FIXED)) {
                vanishDie(pObj, 1);
                vanishFlg[y] |= 1 << x;
                count++;
            }
        }
    }
    vanishFlashFrames = VANISH_FLASH_FRAMES_MAX;
    return count;
}

static void vanishLinkedDice(uint16_t chain)
{
    for (int y = 0; y < FIELD_H; y++) {
        for (int x = 0; x < FIELD_W; x++) {
            if (vanishFlg[y] & 1 << x) {
                OBJ_T *pObj = &field[y][x];
                vanishDie(pObj, chain);
                pObj->depth = (pObj->depth < DEPTH_MAX / 10) ? 0 : pObj->depth - DEPTH_MAX / 10;
            }
        }
    }
    vanishFlashFrames = VANISH_FLASH_FRAMES_MAX;
    currentChain = chain;
    showChainFrames = SHOW_CHAIN_FRAMES_MAX;
}

static void vanishDie(OBJ_T *pObj, uint16_t chain) {
    if (pObj->mode != OBJ_MODE_VANISH) {
        record.erasedDice++;
        norm--;
        if (pObj->mode != OBJ_MODE_APPEAR) countValidDice--;
    }
    pObj->mode = OBJ_MODE_VANISH;
    pObj->chain = chain;
}

static void setLevel(int newLevel)
{
    level = newLevel;
    if (level == 1) norm = 0;

    uint8_t grade = level / 5;
    uint8_t gradeStep = level % 5;
    countDiceMin = min(12 + grade + gradeStep, 28);
    countDiceUsual = min(18 + grade * 2, 29) + gradeStep;
    countDiceMax = 35;
    intervalDieUsual = max(FPS * 4 - grade * 12 - gradeStep * 16, FPS / 2);
    intervalDieMax = FPS * 6 - grade * 5;
    norm += grade * 2 + 6;
    elaspedFrames = min(FPS * (grade - 10) / 2, 0);
    if (gradeStep == 0) elaspedFrames -= FPS * 4;
    dprint(F("Level "));
    dprintln(level);
}

static bool isGameOver(void)
{
    switch (gameMode) {
    case GAME_MODE_ENDLESS:
        return (countValidDice >= FIELD_H * FIELD_W);
    case GAME_MODE_LIMITED:
        return (gameFrames >= FRAMES_3MINUTES);
    case GAME_MODE_PUZZLE:
        return false;
    }
}

static void checkHiscore(void)
{
    if (gameMode == GAME_MODE_ENDLESS) {
        if (record.endlessHiscore < score) {
            record.endlessHiscore = score;
            record.endlessMaxLevel = level;
            dprintln(F("New record! (Endless)"));
        }
    } else if (gameMode == GAME_MODE_LIMITED) {
        if (record.limitedHiscore < score) {
            record.limitedHiscore = score;
            record.limitedMaxChain = maxChain;
            dprintln(F("New record! (Time Limited)"));
        }
    }
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawField(void)
{
    for (int y = 0; y < FIELD_H; y++) {
        for (int x = 0; x < FIELD_W; x++) {
            drawObject(x, y, field[y][x]);
        }
    }
    arduboy.fillRect(106, 0, 4, HEIGHT, BLACK);
}

static void drawObject(int x, int y, OBJ_T obj)
{
    if (obj.type == OBJ_TYPE_BLANK) return;

    int dx = x * 11 + 24;
    int dy = y * 11 + 4;
    bool isFlash = (vanishFlashFrames > 0 && (vanishFlg[y] & 1 << x));

    /*  Floor  */
    if (obj.type == OBJ_TYPE_FLOOR || gameFrames % 2 && !isFlash && obj.depth >= DEPTH_MAX / 2 &&
            (obj.mode == OBJ_MODE_APPEAR || obj.mode == OBJ_MODE_VANISH)) {
        drawFloor(dx + DEPTH_PIXEL, dy + DEPTH_PIXEL);
        return;
    }

    /*  Die  */
    int d = min(obj.depth / (DEPTH_MAX / (DEPTH_PIXEL + 1)), DEPTH_PIXEL);
    dx += d;
    dy += d;
    if (isFlash) {
        drawObjectBitmap(dx, dy, IMG_OBJ_DIE_MASK, WHITE);
    } else {
        bool isWhite = (obj.mode == OBJ_MODE_FIXED || obj.mode == OBJ_MODE_VANISH && !(gameFrames % 2));
        drawDie(dx, dy, obj.type, obj.rotate, isWhite);
    }
}

static void drawFloor(int16_t dx, int16_t dy)
{
    drawObjectBitmap(dx, dy, IMG_OBJ_FLOOR_MASK, BLACK);
    drawObjectBitmap(dx, dy, IMG_OBJ_FLOOR, WHITE);
}

static void drawDie(int16_t dx, int16_t dy, uint16_t type, uint16_t rotate, boolean isWhite)
{
    drawObjectBitmap(dx, dy, IMG_OBJ_DIE_MASK, BLACK);
    int imgId;
    if (isWhite) {
        imgId = pgm_read_byte(imgObjIdMap + (type - OBJ_TYPE_1) * 2 + (rotate % 2));
    } else {
        imgId = (type - OBJ_TYPE_1) * 4 + rotate;
    }
    drawObjectBitmap(dx, dy, imgId, WHITE);
}

static void drawObjectBitmap(int16_t dx, int16_t dy, int imgId, uint8_t c)
{
    arduboy.drawBitmap(dx, dy, imgObj[imgId], IMG_OBJ_W, IMG_OBJ_H, c);
}

static void drawCursor(void)
{
    int dx = cursorX * 11 + 22;
    int dy = cursorY * 11 - 1;
    int imgId = gameFrames % 10;
    if (imgId >= 5) imgId = 10 - imgId;
    arduboy.drawBitmap(dx, dy, imgCursorMask[imgId], 16, 8, BLACK);
    arduboy.drawBitmap(dx, dy, imgCursor[imgId], 16, 8, WHITE);
 
    OBJ_T obj = field[cursorY][cursorX];
    if (obj.type >= OBJ_TYPE_1 && obj.type <= OBJ_TYPE_6) {
        drawDie(108, 0, obj.type, obj.rotate, false);
    }
    arduboy.drawBitmap(109, 17, imgLblInfo, 13, 5, WHITE);
}

static void drawStrings(void)
{
    /*  Score  */
    arduboy.drawBitmap(0, 6, imgLblScore, 19, 5, WHITE);
    drawNumber(0, 0, score);

    if (gameMode == GAME_MODE_LIMITED) {
        /*  Time  */
        arduboy.drawBitmap(0, 53, imgLblTime, 13, 5, WHITE);
        drawTime(0, 59, FRAMES_3MINUTES - gameFrames);
    } else {
        /*  Level  */
        arduboy.drawBitmap(0, 53, imgLblLevel, 19, 5, WHITE);
        drawNumber(0, 59, level);
    }

    if (state == STATE_PLAYING && showChainFrames > 0) {
        /*  Chain  */
        arduboy.drawBitmap(111, 53, imgLblChain, 17, 5, WHITE);
        if (showChainFrames / 4 % 2) {
            int16_t x = 111 + ((currentChain < 100) + (currentChain < 10)) * 6;
            drawNumber(x, 59, currentChain);
        }
    }
}

static void drawLargeLabel(void)
{
#if 0
    arduboy.setTextColor(BLACK);
    arduboy.setTextSize(2);
    arduboy.printEx(10, 26, F("GAME OVER"));
    arduboy.setTextColor(WHITE);
    arduboy.setTextSize(1);
#endif
}
