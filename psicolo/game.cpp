#include "common.h"
#include "image.h"

/*  Defines  */

enum STATE_T {
    STATE_INIT = 0,
    STATE_PLAYING,
    STATE_MENU,
    STATE_OVER,
    STATE_ISSUES,
    STATE_RESULT,
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
#define EACHDIE_MAX 9999
#define SCORE_MAX   9999999

#define COUNT_DICE_INITIAL      15
#define FRAMES_3MINUTES         (FPS * 60 * 3)
#define VANISH_FLASH_FRAMES_MAX 6
#define BLINK_FRAME_FRAMES_MAX  (FPS / 4)
#define BLINK_CHAIN_FRAMES_MAX  (FPS * 5)
#define BLINK_LEVEL_FRAMES_MAX  (FPS * 3)
#define OVER_ANIM_FRAMES_MAX    FPS

#define BYTES_PER_ISSUE 23
#define COUNT_ISSUES    40

/*  Typedefs  */

typedef struct {
    uint8_t     type;   // 0:Floor / 1-6:Dice / 7:Blank
    uint8_t     rotate; // 0-3
    uint8_t     mode;   // 0:Appear / 1:Normal / 2:Fixed / 3:Vanish
    uint16_t    depth;  // 0-320
    uint16_t    chain;  // 0-999
} OBJ_T;

/*  Local Functions  */

static void startGame(void);
static void initTrialField(void);
static void initPuzzleField(void);
static void updateGamePlaying(void);
static void updateGameOver(void);
static void updateGameIssues(void);
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
static bool isTimeToBlinkFrame(void);
static bool isGameOver(void);

static void setupMenu(void);
static void onMenuContine(void);
static void onMenuRestart(void);
static void onMenuBackToTitle(void);
static void onMenuNextIssue(void);
static void onMenuPreviousIssue(void);
static void onMenuSelectIssue(void);

static void drawField(void);
static void drawObject(int x, int y);
static bool checkNeiborDepth(OBJ_T *pObj);
static void drawFloor(int16_t dx, int16_t dy, bool isDrawMask);
static void drawDie(int16_t dx, int16_t dy, uint16_t type, uint16_t rotate, boolean isWhite);
static void drawObjectBitmap(int16_t x, int16_t y, int id, uint8_t c);
static void drawCursor(void);
static void drawStrings(void);
static void drawIssues(void);
static void drawResult(void);
static void drawOverAnimation(void);

/*  Local Variables  */

PROGMEM static const uint8_t mapDieRotate[6 * 4 * 4] = {
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

PROGMEM static const uint8_t mapWhiteDieImgId[6 * 2] = {
     IMG_OBJ_DIE_W_1,  IMG_OBJ_DIE_W_1,  // 1
     IMG_OBJ_DIE_W_2A, IMG_OBJ_DIE_W_2B, // 2
     IMG_OBJ_DIE_W_3A, IMG_OBJ_DIE_W_3B, // 3
     IMG_OBJ_DIE_W_4,  IMG_OBJ_DIE_W_4,  // 4
     IMG_OBJ_DIE_W_5,  IMG_OBJ_DIE_W_5,  // 5
     IMG_OBJ_DIE_W_6A, IMG_OBJ_DIE_W_6B, // 6
};

PROGMEM static const uint8_t imgMiniDie[6][5] = {
    { 0x1F, 0x1F, 0x1B, 0x1F, 0x1F }, // 1
    { 0x1F, 0x1D, 0x1F, 0x17, 0x1F }, // 2
    { 0x1F, 0x17, 0x1B, 0x1D, 0x1F }, // 3
    { 0x1F, 0x15, 0x1F, 0x15, 0x1F }, // 4
    { 0x1F, 0x15, 0x1B, 0x15, 0x1F }, // 5
    { 0x1F, 0x11, 0x1F, 0x11, 0x1F }, // 6
};

PROGMEM static const uint8_t puzzleData[BYTES_PER_ISSUE/* * COUNT_ISSUES*/] = { // TODO
    0xDE, 0x7B, 0xEF, 0xBD, 0xF7, 0xFF, 0xFF, 0xFF, 0xBD, 0xFF, 0xE4, 0x93, 0xEF, 0xFD, 0xFF, 0xFF, 0x7F, 0xEF, 0xBD, 0xF7, 0xDE, 0x7B, 0x03,
};

PROGMEM static const byte soundStart[] = {
    0x90, 72, 0, 100, 0x90, 74, 0, 100, 0x90, 76, 0, 100,
    0x90, 77, 0, 100, 0x90, 79, 0, 200, 0x80, 0xF0
};

static STATE_T  state = STATE_INIT;
static OBJ_T    field[FIELD_H][FIELD_W];
static uint8_t  vanishFlg[FIELD_H];
static uint16_t erasedEachDie[6];

static uint32_t score;
static uint32_t gameFrames;

static int16_t  elaspedFrames;
static uint16_t maxChain;
static uint16_t currentChain;

static int8_t   cursorX, cursorY;
static int8_t   level, norm, step, issue;
static uint8_t  countDice, countValidDice;
static uint8_t  countDiceMin, countDiceUsual, countDiceMax;
static uint8_t  intervalDieUsual, intervalDieMax;
static uint8_t  vanishFlashFrames, blinkFrameFrames, blinkChainFrames, blinkLevelFrames;
static uint8_t  overAnimFrames;

static bool     blinkFlg;
static const __FlashStringHelper *pLargeLabel;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    if (gameMode == GAME_MODE_PUZZLE) {
        issue = 0;
        if (record.puzzleClearCount < COUNT_ISSUES) {
            while (record.puzzleClearFlag[issue / 8] & 1 << issue % 8) issue++;
        }
        state = STATE_ISSUES;
        isInvalid = true;
    } else {
        startGame();
    }
}

MODE_T updateGame(void)
{
    switch (state) {
    case STATE_PLAYING:
        updateGamePlaying();
        break;
    case STATE_OVER:
        updateGameOver();
        break;
    case STATE_ISSUES:
        updateGameIssues();
        break;
    case STATE_MENU:
    case STATE_RESULT:
        handleMenu();
        break;
    }
    return (state == STATE_LEAVE) ? MODE_TITLE : MODE_GAME;
}

void drawGame(void)
{
    if (state == STATE_LEAVE) return;
    if (isInvalid) {
        arduboy.clear();
        if (state == STATE_ISSUES) {
            drawIssues();
        } else if (state == STATE_RESULT) {
            drawResult();
        } else {
            drawField();
            if (state == STATE_PLAYING) {
                drawStrings();
                drawCursor();
            }
            if (blinkFrameFrames > 0 && blinkFlg) arduboy.drawRect(0, 0, WIDTH, HEIGHT, WHITE);
        }
        isInvalid = false;
    }
    if (state == STATE_MENU || state == STATE_RESULT) {
        drawMenuItems(false);
    }
    if (state == STATE_OVER) drawOverAnimation();
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void startGame(void)
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

    memset(erasedEachDie, 0, sizeof(erasedEachDie));
    cursorX = (FIELD_W - 1) / 2;
    cursorY = (FIELD_H - 1) / 2;
    elaspedFrames = 0;
    gameFrames = 0;
    maxChain = 0;
    vanishFlashFrames = 0;
    blinkFrameFrames = 0;
    blinkChainFrames = 0;
    blinkLevelFrames = 0;
    score = 0;
    blinkFlg = false;
    state = STATE_PLAYING;
    isRecordDirty = true;
    isInvalid = true;
    arduboy.playScore2(soundStart, 0);
}

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
    countDice = 0;
    const uint8_t *p = puzzleData; // +issue * BYTES_PER_ISSUE; // TODO
    uint16_t buffer = 0, bits = 0;
    for (int y = 0; y < FIELD_H; y++) {
        for (int x = 0; x < FIELD_W; x++) {
            if (bits < 5) {
                buffer += pgm_read_byte(p++) << bits;
                bits += 8;
            }
            OBJ_T *pObj = &field[y][x];
            uint16_t data = buffer & 0x1F;
            if (data < 30) {
                if (data < 24) {
                    pObj->type = data / 4 + OBJ_TYPE_1;
                    pObj->rotate = data % 4;
                    pObj->mode = OBJ_MODE_NORMAL;
                } else {
                    pObj->type = data - 24 + OBJ_TYPE_1;
                    pObj->rotate = 0;
                    pObj->mode = OBJ_MODE_FIXED;
                }
                pObj->chain = 0;
                pObj->depth = 0;
                countDice++;
            } else {
                pObj->type = (data == 30) ? OBJ_TYPE_BLANK : OBJ_TYPE_FLOOR;
                pObj->depth = DEPTH_MAX;
            }
            buffer >>= 5;
            bits -= 5;
        }
    }
    countValidDice = countDice;
    step = pgm_read_byte(p);
    dprint(F("Puzzle initialized "));
    dprintln(issue);
}

static void updateGamePlaying(void)
{
#ifdef DEBUG
    if (dbgRecvChar == 'a') initTrialField();
    if (dbgRecvChar == 'l') norm = 0;
    if (dbgRecvChar == 'q') field[cursorY][cursorX].chain = CHAIN_MAX;
#endif
    handleDPad();
    updateField();
    moveCursor();
    if (gameMode == GAME_MODE_ENDLESS) {
        if (level < LEVEL_MAX && norm <= 0) setLevel(level + 1);
    } else if (gameMode == GAME_MODE_LIMITED) {
        if (isTimeToBlinkFrame()) blinkFrameFrames = BLINK_FRAME_FRAMES_MAX;
    }
    if (vanishFlashFrames > 0) vanishFlashFrames--;
    if (blinkFrameFrames > 0) blinkFrameFrames--;
    if (blinkChainFrames > 0) blinkChainFrames--;
    if (blinkLevelFrames > 0) blinkLevelFrames--;
    gameFrames++;
    record.playFrames++;
    blinkFlg = !blinkFlg;
    if (isGameOver()) {
        state = STATE_OVER;
        writeRecord();
        blinkFlg = false;
        overAnimFrames = OVER_ANIM_FRAMES_MAX;
        dprintln(F("Game over"));
    } else if (arduboy.buttonDown(A_BUTTON)) {
        setupMenu();
    }
    isInvalid = true;
}

static void updateGameOver(void)
{
    if (overAnimFrames > 0) {
        overAnimFrames--;
    } else if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
        if (gameMode == GAME_MODE_PUZZLE) {
            state = STATE_ISSUES;
            playSoundClick();
        } else {
            setupMenu();
        }
        isInvalid = true;
    }
}

static void updateGameIssues(void)
{
    handleDPad();
    if (issue == COUNT_ISSUES && padX > 0) padX = 0;
    if (padX != 0 || padY != 0) {
        int tmp = issue + padX + padY * 5;
        if (tmp >= 0 && tmp <= COUNT_ISSUES + 4) {
            issue = min(tmp, COUNT_ISSUES);
            playSoundTick();
            isInvalid = true;
        }
    }
    if (arduboy.buttonDown(A_BUTTON)) {
        setSound(!arduboy.audio.enabled());
        playSoundClick();
        isInvalid = true;
    }
    if (arduboy.buttonDown(B_BUTTON)) {
        if (issue == COUNT_ISSUES) {
            state = STATE_LEAVE;
            playSoundClick();
        } else {
            startGame();
        }
    }
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
                if (blinkFlg) {
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
                playSoundTick();
                step--;
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
    int tmpId = pgm_read_byte(mapDieRotate + idx);
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
            score += (uint32_t) die.type * link * chain;
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
    blinkChainFrames = BLINK_CHAIN_FRAMES_MAX;
}

static void vanishDie(OBJ_T *pObj, uint16_t chain) {
    if (pObj->mode != OBJ_MODE_VANISH) {
        erasedEachDie[pObj->type - OBJ_TYPE_1]++;
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
    blinkFrameFrames = BLINK_FRAME_FRAMES_MAX;
    blinkLevelFrames = BLINK_LEVEL_FRAMES_MAX;
    dprint(F("Level "));
    dprintln(level);
    dprintln(countDiceMin);
    dprintln(countDiceUsual);
    dprintln(intervalDieUsual);
    dprintln(intervalDieMax);
    dprintln(elaspedFrames);
}

static bool isTimeToBlinkFrame(void)
{
    return (gameFrames == FPS * 60  || gameFrames == FPS * 120 ||
            gameFrames == FPS * 150 || gameFrames == FPS * 160 ||
            gameFrames % FPS == 0 && gameFrames >= FPS * 170 && gameFrames < FRAMES_3MINUTES);
}

static bool isGameOver(void)
{
#ifdef DEBUG
    if (dbgRecvChar == 'o') {
        pLargeLabel = F("DEBUG");
        return true;
    }
#endif
    switch (gameMode) {
    case GAME_MODE_ENDLESS:
        if (countValidDice >= FIELD_H * FIELD_W) {
            pLargeLabel = F("GAME OVER");
            if (record.endlessHiscore < score) {
                record.endlessHiscore = score;
                record.endlessMaxLevel = level;
                dprintln(F("New record! (Endless)"));
            }
            return true;
        }
        return false;
    case GAME_MODE_LIMITED:
        if (gameFrames >= FRAMES_3MINUTES) {
            pLargeLabel = F("TIME UP");
            if (record.limitedHiscore < score) {
                record.limitedHiscore = score;
                record.limitedMaxChain = maxChain;
                dprintln(F("New record! (Time Limited)"));
            }
            return true;
        }
        return false;
    case GAME_MODE_PUZZLE:
        if (step <= 0 || countValidDice == 0) {
            if (countValidDice == 0) {
                pLargeLabel = F("CLEAR");
                int idx = issue / 8;
                uint8_t bit = 1 << issue % 8;
                if ((record.puzzleClearFlag[idx] & bit) == 0) {
                    record.puzzleClearFlag[idx] |= bit;
                    record.puzzleClearCount++;
                    dprint(F("Clear puzzle issue "));
                    dprintln(issue);
                    if (issue < COUNT_ISSUES - 1) issue++;
                }
            } else {
                pLargeLabel = F("AGAIN");
            }
            return true;
        }
        return false;
    }
}

static void setupMenu(void)
{
    clearMenuItems();
    if (state == STATE_PLAYING) {
        addMenuItem(F("CONTINUE"), onMenuContine);
    }
    addMenuItem(F("RESTART"), onMenuRestart);
    if (gameMode == GAME_MODE_PUZZLE) {
        if (issue < COUNT_ISSUES - 1) addMenuItem(F("NEXT ISSUE"), onMenuNextIssue);
        if (issue > 0) addMenuItem(F("PREV ISSUE"), onMenuPreviousIssue);
        addMenuItem(F("ISSUES LIST"), onMenuSelectIssue);
    } else {
        addMenuItem(F("BACK TO TITLE"), onMenuBackToTitle);
    }

    if (state == STATE_OVER) {
        setMenuCoords(19, 53, 89, 11, false, false);
        state = STATE_RESULT;
    } else {
        int h = getMenuItemCount() * 6 - 1;
        setMenuCoords(19, 32 - h / 2, 89, h, true, true);
        state = STATE_MENU;
    }
    setMenuItemPos(0);
    playSoundClick();
}

static void onMenuContine(void)
{
    playSoundClick();
    state = STATE_PLAYING;
}

static void onMenuRestart(void)
{
    writeRecord();
    startGame();
}

static void onMenuBackToTitle(void)
{
    playSoundClick();
    writeRecord();
    state = STATE_LEAVE;
}

static void onMenuNextIssue(void)
{
    issue++;
    onMenuRestart();
}

static void onMenuPreviousIssue(void)
{
    issue--;
    onMenuRestart();
}

static void onMenuSelectIssue(void)
{
    playSoundClick();
    writeRecord();
    state = STATE_ISSUES;
    isInvalid = true;
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawField(void)
{
    if (gameMode == GAME_MODE_LIMITED && state == STATE_MENU) {
        /*  Dummy  */
        for (int i = 0; i <= FIELD_W; i++) {
            arduboy.drawFastVLine2(i * 11 + 28, 8, 56, WHITE);
            arduboy.drawFastHLine2(28, i * 11 + 8, 78, WHITE);
        }
    } else {
        /*  Normal case  */
        for (int y = 0; y < FIELD_H; y++) {
            for (int x = 0; x < FIELD_W; x++) {
                drawObject(x, y);
            }
        }
        arduboy.fillRect(106, 0, 4, HEIGHT, BLACK);
    }
}

static void drawObject(int x, int y)
{
    OBJ_T *pObj = &field[y][x];
    if (pObj->type == OBJ_TYPE_BLANK) return;

    int dx = x * 11 + 24;
    int dy = y * 11 + 4;
    bool isFlash = (vanishFlashFrames > 0 && (vanishFlg[y] & 1 << x));

    /*  Floor  */
    if (pObj->type == OBJ_TYPE_FLOOR ||
            blinkFlg && !isFlash && pObj->depth >= DEPTH_MAX / 2 &&
            (pObj->mode == OBJ_MODE_APPEAR || pObj->mode == OBJ_MODE_VANISH)) {
        bool isDrawMask = (x > 0 && checkNeiborDepth(&field[y][x - 1])) ||
                (y > 0 && checkNeiborDepth(&field[y - 1][x])) ||
                (x > 0 && y > 0 && checkNeiborDepth(&field[y - 1][x - 1]));
        drawFloor(dx + DEPTH_PIXEL, dy + DEPTH_PIXEL, isDrawMask);
        return;
    }

    /*  Die  */
    int d = min(pObj->depth / (DEPTH_MAX / (DEPTH_PIXEL + 1)), DEPTH_PIXEL);
    dx += d;
    dy += d;
    if (isFlash) {
        drawObjectBitmap(dx, dy, IMG_OBJ_DIE_MASK, WHITE);
    } else {
        bool isWhite = (pObj->mode == OBJ_MODE_FIXED ||
                pObj->mode == OBJ_MODE_VANISH && !blinkFlg);
        drawDie(dx, dy, pObj->type, pObj->rotate, isWhite);
    }
}

static bool checkNeiborDepth(OBJ_T *pObj)
{
    return (pObj->type >= OBJ_TYPE_1 && pObj->type <= OBJ_TYPE_6 &&
        pObj->depth >= DEPTH_MAX / (DEPTH_PIXEL + 1));
}

static void drawFloor(int16_t dx, int16_t dy, bool isDrawMask)
{
    if (isDrawMask) arduboy.fillRect(dx + 1, dy + 1, 10, 10, BLACK);
    arduboy.drawRect(dx, dy, 12, 12, WHITE);
}

static void drawDie(int16_t dx, int16_t dy, uint16_t type, uint16_t rotate, boolean isWhite)
{
    drawObjectBitmap(dx, dy, IMG_OBJ_DIE_MASK, BLACK);
    int imgId;
    if (isWhite) {
        imgId = pgm_read_byte(mapWhiteDieImgId + (type - OBJ_TYPE_1) * 2 + (rotate % 2));
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
    int dx = cursorX * 11 + 24;
    int dy = cursorY * 11 - 1;
    int imgId = gameFrames % 10;
    if (imgId >= 5) imgId = 10 - imgId;
    arduboy.drawBitmap(dx - 1, dy, imgCursorMask[imgId], 14, 8, BLACK);
    arduboy.drawBitmap(dx, dy, imgCursor[imgId], 12, 8, WHITE);
 
    OBJ_T obj = field[cursorY][cursorX];
    if (obj.type >= OBJ_TYPE_1 && obj.type <= OBJ_TYPE_6) {
        drawDie(108, 0, obj.type, obj.rotate, false);
    }
    arduboy.drawBitmap(109, 17, imgLabelInfo, 13, 5, WHITE);
}

static void drawStrings(void)
{
    if (gameMode == GAME_MODE_PUZZLE) {
        /*  Step  */
        arduboy.drawBitmap(0, 0, imgLabelStep, 15, 5, WHITE);
        drawNumber(0, 6, step);
        /*  Issue  */
        arduboy.drawBitmap(0, 53, imgLabelIssue, 17, 5, WHITE);
        drawNumber(0, 59, issue + 1);
    } else {
        /*  Score  */
        arduboy.drawBitmap(0, 6, imgLabelScore, 19, 5, WHITE);
        arduboy.setTextColor(WHITE);
        drawNumber(0, 0, score);
        arduboy.setTextColor(WHITE, WHITE);
        if (gameMode == GAME_MODE_ENDLESS) {
            /*  Level  */
            arduboy.drawBitmap(0, 53, imgLabelLevel, 19, 5, WHITE);
            if (blinkLevelFrames / 4 % 2 == 0) drawNumber(0, 59, level);
        } else {
            /*  Time  */
            arduboy.drawBitmap(0, 53, imgLabelTime, 13, 5, WHITE);
            drawTime(0, 59, FRAMES_3MINUTES - gameFrames);
        }
    }

    if (state == STATE_PLAYING && blinkChainFrames > 0) {
        /*  Chain  */
        arduboy.drawBitmap(111, 53, imgLabelChain, 17, 5, WHITE);
        if (blinkChainFrames / 4 % 2 == 0) {
            int16_t x = 111 + ((currentChain < 100) + (currentChain < 10)) * 6;
            drawNumber(x, 59, currentChain);
        }
    }
}

static void drawIssues(void)
{
    arduboy.printEx(34, 0, F("[ PUZZLE ]"));
    for (int i = 0; i <= COUNT_ISSUES; i++) {
        int dx = i % 5 * 24 + 10, dy = i / 5 * 6 + 8;
        if (i == issue) {
            dx -= 4;
            arduboy.fillRect(dx - 6, dy, 5, 5, WHITE);
        }
        if (i == COUNT_ISSUES) {
            arduboy.printEx(dx, dy, F("BACK TO TITLE"));
        } else {
            drawNumber(dx, dy, i + 1);
            if (record.puzzleClearFlag[i / 8] & 1 << i % 8) {
                arduboy.drawFastHLine(dx - 2, dy + 2, 15 - (i < 9) * 6, WHITE);
            }
        }
    }
    drawSoundEnabled();
}

static void drawResult(void)
{
    /*  Score  */
    arduboy.setTextSize(2);
    int dx = 59;
    uint32_t dummy = score;
    while (dummy >= 10) {
        dummy /= 10;
        dx -= 6;
    }
    drawNumber(dx, 0, score);
    arduboy.setTextSize(1);

    /*  Each die  */
    uint32_t totalErasedDice = 0;
    for (int i = 0; i < 6; i++) {
        int dx = (i % 3) * 30 + 37;
        int dy = (i / 3) * 6 + 38;
        arduboy.drawBitmap(dx, dy, imgMiniDie[i], 5, 5, WHITE);
        drawNumber(dx + 6, dy, min(erasedEachDie[i], EACHDIE_MAX));
        totalErasedDice += erasedEachDie[i];
    }

    /*  Others  */
    arduboy.printEx(1, 14, F("LEVEL"));
    arduboy.printEx(1, 20, F("MAX CHAIN"));
    arduboy.printEx(1, 26, F("PLAY TIME"));
    arduboy.printEx(1, 32, F("ERASED DICE"));
    if (gameMode == GAME_MODE_ENDLESS) {
        drawNumber(73, 14, level);
    } else {
        arduboy.printEx(73, 14, F("--"));
    }
    drawNumber(73, 20, maxChain);
    drawTime(73, 26, gameFrames);
    drawNumber(73, 32, totalErasedDice);
}

static void drawOverAnimation(void)
{
    if (overAnimFrames == 0) return;

    /*  Shake screen  */
    if (overAnimFrames > OVER_ANIM_FRAMES_MAX / 2) {
        if (gameMode != GAME_MODE_PUZZLE) {
            arduboy.sendLCDCommand(0x40 + !(overAnimFrames % 2) * 4);
        }
        return;
    }

    /*  Large letters  */
    int w = min(OVER_ANIM_FRAMES_MAX / 2 - overAnimFrames, 6);
    arduboy.drawRect(0, 31 - w, WIDTH, w * 2 + 2, WHITE);
    arduboy.fillRect(0, 32 - w, WIDTH, w * 2, BLACK);
    if (overAnimFrames <= OVER_ANIM_FRAMES_MAX / 2 - 6) {
        int dx = 64 - strlen_P((const char *)pLargeLabel) * 6;
        dx += overAnimFrames * overAnimFrames / 2;
        arduboy.setTextSize(2);
        arduboy.printEx(dx, 27, pLargeLabel);
        arduboy.setTextSize(1);
    }
}
