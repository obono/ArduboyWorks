#include "common.h"
#include "image.h"

/*  Defines  */

enum STATE_T {
    STATE_INIT = 0,
    STATE_GAME,
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
#define CHAIN_MAX   999

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

static void updateField();
static void updateObject(OBJ_T *pObj);
static OBJ_T *chooseFloor(void);
static void setDie(OBJ_T *pDie, OBJ_MODE mode);
static inline void rotateDie(OBJ_T *pDie, int vx, int vy);
static void moveCursor(void);
static void judgeVanish(int x, int y);
static bool judgeHappyOne(int x, int y);
static int  judgeLinkedDice(int x, int y, uint16_t type, uint16_t *pMaxChain);
static int  vanishHappyOne(void);
static void vanishLinkedDice(uint16_t chain);

static void drawField(void);
static void drawObject(int x, int y, OBJ_T obj);
static inline void drawFloor(int16_t dx, int16_t dy);
static inline void drawDie(int16_t dx, int16_t dy, uint16_t type, uint16_t rotate, boolean isWhite);
static inline void drawObjectBitmap(int16_t x, int16_t y, int id, uint8_t c);
static void drawCursor(void);
static void drawStrings(void);

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
     IMG_OBJ_DIE_W_1,  IMG_OBJ_DIE_W_1,     IMG_OBJ_DIE_W_2A, IMG_OBJ_DIE_W_2B,
     IMG_OBJ_DIE_W_3A, IMG_OBJ_DIE_W_3B,    IMG_OBJ_DIE_W_4,  IMG_OBJ_DIE_W_4,
     IMG_OBJ_DIE_W_5,  IMG_OBJ_DIE_W_5,     IMG_OBJ_DIE_W_6A, IMG_OBJ_DIE_W_6B,
};

PROGMEM static const byte soundScore[] = {
    0x90, 80, 0, 20, 0x90, 84, 0, 40, 0x90, 88, 0, 40, 0x80, 0xF0
};

static STATE_T  state = STATE_INIT;
static OBJ_T    field[FIELD_H][FIELD_W];
static uint8_t  vanishFlg[FIELD_H];

static int8_t   cursorX, cursorY;
static uint8_t  countDice;
static uint8_t  vanishFlash;
static uint16_t appearInterval;
static uint32_t gameFrames;
static uint32_t score;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    OBJ_T floor;
    floor.type = OBJ_TYPE_FLOOR;
    floor.depth = DEPTH_MAX;
    for (int y = 0; y < FIELD_H; y++) {
        for (int x = 0; x < FIELD_W; x++) {
            field[y][x] = floor;
        }
        vanishFlg[y] = 0;
    }

    countDice = 0;
    for (int i = 0; i < 15/*TODO*/; i++) {
        setDie(chooseFloor(), OBJ_MODE_NORMAL);
    }

    cursorX = (FIELD_W - 1) / 2;
    cursorY = (FIELD_H - 1) / 2;
    vanishFlash = 0;
    appearInterval = FPS * 3; // TODO
    gameFrames = 0;
    score = 0;
    state = STATE_GAME;
}

MODE_T updateGame(void)
{
    handleDPad();
    updateField();
    moveCursor();

    if (vanishFlash) vanishFlash--;
    if (arduboy.buttonDown(A_BUTTON)) {
        state = STATE_LEAVE;
    }
    gameFrames++;
    playFrames++;

    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    arduboy.clear();
    drawField();
    drawCursor();
    drawStrings();
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void updateField()
{
    for (int y = 0; y < FIELD_H; y++) {
        for (int x = 0; x < FIELD_W; x++) {
            updateObject(&field[y][x]);
        }
    }
    if (countDice < 8 || countDice < 30 && --appearInterval == 0) {
        setDie(chooseFloor(), OBJ_MODE_APPEAR);
        appearInterval = FPS * 3; // TODO
    }
}

static void updateObject(OBJ_T *pObj)
{
    if (pObj->type == OBJ_TYPE_FLOOR || pObj->type == OBJ_TYPE_BLANK) {
        return;
    }
    switch (pObj->mode) {
    case OBJ_MODE_APPEAR:
        pObj->depth -= 4;
        if (pObj->depth == 0) {
            pObj->mode = OBJ_MODE_NORMAL;
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

static OBJ_T *chooseFloor(void)
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
}

static inline void rotateDie(OBJ_T *pDie, int vx, int vy)
{
    int idx = (pDie->type - OBJ_TYPE_1) * 16 + pDie->rotate * 4 + (vy * 3 + vx + 3) / 2;
    int tmpId = pgm_read_byte(dieRotateMap + idx);
    pDie->type = tmpId / 4 + OBJ_TYPE_1;
    pDie->rotate = tmpId % 4;
}

static void moveCursor(void)
{
    if (cursorX + padX < 0 || cursorX + padX >= FIELD_W) padX = 0;
    if (cursorY + padY < 0 || cursorY + padY >= FIELD_H) padY = 0;
    if (padX != 0 || padY != 0) {
        OBJ_T cursorObj = field[cursorY][cursorX];
        if (arduboy.buttonPressed(B_BUTTON) && cursorObj.type >= OBJ_TYPE_1 &&
                cursorObj.type <= OBJ_TYPE_6 && cursorObj.depth == 0) {
            if (padX != 0 && padY != 0) padY = 0; // TODO
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

static void judgeVanish(int x, int y)
{
    memset(vanishFlg, 0, FIELD_H);
    OBJ_T die = field[y][x];
    if (die.type == OBJ_TYPE_1) {
        if (judgeHappyOne(x, y)) {
            int link = vanishHappyOne();
            score += link;
        }
    } else {
        uint16_t maxChain;
        int link = judgeLinkedDice(x, y, die.type, &maxChain);
        if (link >= die.type) {
            if (maxChain < CHAIN_MAX) maxChain++;
            vanishLinkedDice(maxChain);
            score += die.type * link * maxChain;
        }
    }
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

static int judgeLinkedDice(int x, int y, uint16_t type, uint16_t *pMaxChain)
{
    int link = 0, stackPos = 0;
    uint8_t stack[FIELD_W * FIELD_H + 1];
    uint8_t dir = 1;
    *pMaxChain = 0;
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
            *pMaxChain = max(*pMaxChain, field[y][x].chain);
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
                count++;
                pObj->mode = OBJ_MODE_VANISH;
                pObj->chain = 1;
                vanishFlg[y] |= 1 << x;
            }
        }
    }
    vanishFlash = 5;
    return count;
}

static void vanishLinkedDice(uint16_t chain)
{
    for (int y = 0; y < FIELD_H; y++) {
        for (int x = 0; x < FIELD_W; x++) {
            if (vanishFlg[y] & 1 << x) {
                OBJ_T *pObj = &field[y][x];
                pObj->mode = OBJ_MODE_VANISH;
                pObj->depth = (pObj->depth < DEPTH_MAX / 10) ? 0 : pObj->depth - DEPTH_MAX / 10;
                pObj->chain = chain;
            }
        }
    }
    vanishFlash = 5;
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
}

static void drawObject(int x, int y, OBJ_T obj)
{
    if (obj.type == OBJ_TYPE_BLANK) {
        return;
    }

    int dx = x * 11 + 21;
    int dy = y * 11 + 4;
    bool isFlash = (vanishFlash > 0 && (vanishFlg[y] & 1 << x));

    /* Floor */
    if (obj.type == OBJ_TYPE_FLOOR || gameFrames % 2 && !isFlash && obj.depth >= DEPTH_MAX / 2 &&
            (obj.mode == OBJ_MODE_APPEAR || obj.mode == OBJ_MODE_VANISH)) {
        drawFloor(dx + DEPTH_PIXEL, dy + DEPTH_PIXEL);
        return;
    }

    /* Die */
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

static inline void drawFloor(int16_t dx, int16_t dy)
{
    drawObjectBitmap(dx, dy, IMG_OBJ_FLOOR_MASK, BLACK);
    drawObjectBitmap(dx, dy, IMG_OBJ_FLOOR, WHITE);
}

static inline void drawDie(int16_t dx, int16_t dy, uint16_t type, uint16_t rotate, boolean isWhite)
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

static inline void drawObjectBitmap(int16_t dx, int16_t dy, int imgId, uint8_t c)
{
    arduboy.drawBitmap(dx, dy, imgObj[imgId], IMG_OBJ_W, IMG_OBJ_H, c);
}


static void drawCursor(void)
{
    int dx = cursorX * 11 + 19;
    int dy = cursorY * 11 - 1;
    int imgId = gameFrames % 10;
    if (imgId >= 5) imgId = 10 - imgId;
    arduboy.drawBitmap(dx, dy, imgCursorMask[imgId], 16, 8, BLACK);
    arduboy.drawBitmap(dx, dy, imgCursor[imgId], 16, 8, WHITE);
 
    OBJ_T obj = field[cursorY][cursorX];
    if (obj.type >= OBJ_TYPE_1 && obj.type <= OBJ_TYPE_6) {
        drawDie(108, 0, obj.type, obj.rotate, false);
    }
}

static void drawStrings(void)
{
    arduboy.setCursor(0, 0);
    arduboy.print(score);
    arduboy.setCursor(0, 59);
    arduboy.print(gameFrames / FPS);
}

