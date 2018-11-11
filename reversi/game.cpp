#include "common.h"
#include "data.h"

/*  Defines  */

#define BOARD_W 8
#define BOARD_H 8

enum STATE_T {
    STATE_INIT = 0,
    STATE_PLAYING,
    STATE_ANIMATION,
    STATE_MENU,
    STATE_OVER,
    STATE_LEAVE,
};

enum FIX_COND_T {
    EXIST_EMPTY,
    NO_EMPTY,
    NEIGHBOR_FIXED,
};

/*  Typedefs  */

typedef struct {
    uint8_t black[BOARD_H];
    uint8_t white[BOARD_H];
    uint8_t flag[BOARD_H];
    uint8_t numStones, numBlack, numWhite, numFixedBlack, numFixedWhite, numPlaceable;
    bool    isWhiteTurn, isLastPassed;
} BOARD_T;

typedef struct {
    uint8_t x:3;
    uint8_t y:3;
} POS_T;

/*  Local Functions  */

static void resetAnimationParams(void);
static bool isCpuTurn(void);
static void handlePlaying(void);
static void handleAnimation(void);
static void handleOver(void);
static void onContinue(void);
static void onSuspend(void);
static void onAbandon(void);

static void drawBoard(bool isAnimation);
static void drawStone(int8_t x, int8_t y, int8_t p);
static void drawFlippingStone(int8_t x, int8_t y, int8_t anim, bool isBlack);
static void drawCursor(void);
static void drawPlayerIndication(bool isAnimation);
static void drawStonesCounter(void);
static void draw2Numbers(int16_t x, int16_t y, int8_t value);
static void drawResult(void);

static void analyzeBoard(BOARD_T *p);
static bool isPlaceable(BOARD_T *p, int8_t x, int8_t y, bool isActual);
static bool isReversible(BOARD_T *p, int8_t x, int8_t y, int8_t vx, int8_t vy, bool isActual);
static void checkFixedStones(BOARD_T *p);
static bool isFixed(BOARD_T *p, int8_t x, int8_t y, bool isCheckingBlack);
static FIX_COND_T checkFixCond(BOARD_T *p, int8_t x, int8_t y, int8_t vx, int8_t vy, bool isCheckingBlack);
static bool isGameOver(BOARD_T *p);
#define     placeStone(b, x, y)     isPlaceable(b, x, y, true)
#define     countBits(val)          pgm_read_byte(bitNumTable + (val))

static void cpuThinking(void);
static int  alphabeta(BOARD_T *p, int8_t depth, int alpha, int beta);
static int  evaluateBoard(BOARD_T *p);
static int8_t evaluateBit(const int8_t *pTable, uint8_t value);

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static BOARD_T  board;
static int8_t   animTable[BOARD_H][BOARD_W];
static uint8_t  animStones, animCounter;
static uint8_t  ledRGB[3];
static int      thinkLed;

static POS_T    cursorPos, cursorLastPos;
static uint8_t  cursorBuf[11];
static bool     isCursorMoved;

static uint32_t gameFrames;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    if (record.canContinue) {
        record.canContinue = false;
        memcpy(board.black, record.black, BOARD_H * 2);
        board.isWhiteTurn = record.isWhiteTurn;
        board.isLastPassed = record.isLastPassed;
        analyzeBoard(&board);
        state = STATE_PLAYING;
    } else {
        record.playCount++;
        memset(&board, 0, sizeof(board));
        board.isWhiteTurn = true; // trick
        state = STATE_ANIMATION;
    }

    clearMenuItems();
    addMenuItem(F("CONTINUE"), onContinue);
    addMenuItem(F("CANCEL LAST STEP"), onContinue); // TODO
    addMenuItem(F("SUSPEND GAME"), onSuspend);
    addMenuItem(F("ABANDON GAME"), onAbandon);
    setMenuCoords(10, 20, 108, 24, true, true);

    isRecordDirty = true;
    writeRecord();
    gameFrames = 0;
    resetAnimationParams();
    arduboy.playScore2(soundStart, 0);
    isInvalid = true;
}

MODE_T updateGame(void)
{
    if (state == STATE_PLAYING || state == STATE_ANIMATION) {
        gameFrames++;
        record.playFrames++;
        isRecordDirty = true;
    }
    switch (state) {
    case STATE_PLAYING:
        handlePlaying();
        break;
    case STATE_ANIMATION:
        handleAnimation();
        break;
    case STATE_MENU:
        handleMenu();
        break;
    case STATE_OVER:
        handleOver();
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
        drawBoard(false);
        drawPlayerIndication(false);
        drawStonesCounter();
        drawCursor();
        isInvalid = false;
    } else {
        switch (state) {
        case STATE_PLAYING:
            drawCursor();
            drawPlayerIndication(true);
            break;
        case STATE_ANIMATION:
            drawBoard(true);
            drawPlayerIndication(true);
            if (animCounter % 4 == 0) drawStonesCounter();
            break;
        case STATE_OVER:
            drawResult();
            break;
        }
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void resetAnimationParams(void)
{
    memset(animTable, 0, sizeof(animTable));
    animStones = 0;
    animCounter = 0;
    cursorLastPos = cursorPos;
    isCursorMoved = true;
}

static bool isCpuTurn(void)
{
    return  board.isWhiteTurn && record.gameMode == GAME_MODE_BLACK ||
            !board.isWhiteTurn && record.gameMode == GAME_MODE_WHITE;
}

static void handlePlaying(void)
{
    if (board.numPlaceable == 0) {
        /*  Pass  */
        if (arduboy.buttonDown(B_BUTTON)) {
            if (board.isLastPassed) {
                writeRecord();
                arduboy.playScore2(soundOver, 1);
                state = STATE_OVER;
            } else {
                playSoundClick();
                board.isWhiteTurn = !board.isWhiteTurn;
                board.isLastPassed = true;
                analyzeBoard(&board);
            }
            isInvalid = true;
        }
    } else if (isCpuTurn()) {
        /*  CPU thinking  */
        cpuThinking();
        resetAnimationParams();
        placeStone(&board, cursorPos.x, cursorPos.y);
        playSoundClick();
        state = STATE_ANIMATION;
    } else {
        /*  Move cursor  */
        handleDPad();
        if (padX != 0 || padY != 0) {
            cursorPos.x += padX;
            cursorPos.y += padY;
        }
        /*  Place a stone  */
        if (arduboy.buttonDown(B_BUTTON)) {
            uint8_t empties = ~(board.black[cursorPos.y] | board.white[cursorPos.y]);
            if (empties & board.flag[cursorPos.y] & 1 << cursorPos.x) {
                placeStone(&board, cursorPos.x, cursorPos.y);
                playSoundClick();
                state = STATE_ANIMATION;
            }
        }
    }

    if (arduboy.buttonDown(A_BUTTON)) {
        setMenuItemPos(0);
        state = STATE_MENU;
        isInvalid = true;
    }
}

static void handleAnimation(void)
{
    uint8_t animCounterMax = (animStones > 0) ? animStones * 4 + 32 : 20;
    if (++animCounter < animCounterMax) {
        /*  Flipping animation  */
        if (animCounter % 4 == 0 && animCounter >= 20 && animCounter < animStones * 4 + 20) {
            if (board.isWhiteTurn) {
                board.numBlack--;
                board.numWhite++;
            } else {
                board.numBlack++;
                board.numWhite--;
            }
            playSoundTick();
        }
    } else {
        /*  Switch player turn  */
        board.isWhiteTurn = !board.isWhiteTurn;
        board.isLastPassed = false;
        analyzeBoard(&board);
        resetAnimationParams();
        int numStones = board.numStones;
        if (numStones < 4) {
            int8_t x = (numStones == 0 || numStones == 3) ? 3 : 4;
            int8_t y = (numStones == 0 || numStones == 1) ? 3 : 4;
            placeStone(&board, x, y);
            playSoundClick();
        } else if (isGameOver(&board)) {
            writeRecord();
            arduboy.playScore2(soundOver, 1);
            state = STATE_OVER;
        } else {
            state = STATE_PLAYING;
        }
        isInvalid = true;
    }
}

static void handleOver(void)
{
    if (arduboy.buttonDown(A_BUTTON)) {
        state = STATE_LEAVE;
    }
}

static void onContinue(void)
{
    playSoundClick();
    state = STATE_PLAYING;
    isInvalid = true;
}

static void onSuspend(void)
{
    playSoundClick();
    memcpy(record.black, board.black, BOARD_H * 2);
    record.isWhiteTurn = board.isWhiteTurn;
    record.isLastPassed = board.isLastPassed;
    record.canContinue = true;
    writeRecord();
    state = STATE_LEAVE;
    dprintln(F("Suspend this game"));
}

static void onAbandon(void)
{
    playSoundClick();
    writeRecord();
    state = STATE_LEAVE;
    dprintln(F("Abandon this game"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawBoard(bool isAnimation)
{
    for (int8_t y = 0; y < BOARD_H; y++) {
        uint8_t black = board.black[y];
        uint8_t white = board.white[y];
        uint8_t flag = board.flag[y];
        uint8_t placeable = flag & ~(black | white);
        uint8_t fixed = flag & (black | white);
        if ((record.settings & SETTING_BIT_SHOW_PLACEABLE) == 0 || state != STATE_PLAYING) placeable = 0;
        if ((record.settings & SETTING_BIT_SHOW_FIXED) == 0) fixed = 0;
        int dy = y * 8;
        int anim = -1;
        for (int8_t x = 0; x < BOARD_W; x++) {
            int dx = x * 12 + 16;
            uint8_t b = 1 << x;
            if (isAnimation) {
                anim = animTable[y][x] - animCounter;
                int belowAnim = (y < BOARD_H - 1) ? animTable[y + 1][x] - animCounter : -1;
                if ((anim < 0 || anim >= 16) && (belowAnim < 0 || belowAnim >= 16)) continue; // Skip drawing
            }
            if (anim <= 0) {
                if (black & b) {
                    drawStone(dx, y, IMG_ID_BLACK);
                } else if (white & b) {
                    drawStone(dx, y, IMG_ID_WHITE);
                } else {
                    drawStone(dx, y, (placeable & b) ? IMG_ID_PLACEABLE : IMG_ID_EMPTY);
                }
                if (fixed & b) {
                    arduboy.drawPixel(dx + 5, dy + 4, (black & b) ? WHITE : BLACK);
                }
            } else if (anim < 16) {
                drawStone(dx, y, IMG_ID_EMPTY);
                drawFlippingStone(dx, dy, anim, !board.isWhiteTurn);
            } else {
                drawStone(dx, y, board.isWhiteTurn ? IMG_ID_BLACK : IMG_ID_WHITE);
            }
        }
    }
}

static void drawStone(int8_t dx, int8_t y, int8_t p) {
    memcpy_P(arduboy.getBuffer() + dx + y * WIDTH, imgStone[p], 11);
}

static void drawFlippingStone(int8_t dx, int8_t dy, int8_t anim, bool isBlack) {
    arduboy.drawBitmap(dx, dy - 8, imgFlippingBase[anim], 11, 16, WHITE);
    if (anim > 4) isBlack = !isBlack;
    if (isBlack) {
        arduboy.drawBitmap(dx, dy - 8, imgFlippingFace[anim], 11, 16, BLACK);
    }
}

static void drawCursor(void) {
    if (board.numStones < 4 || board.numPlaceable == 0 || isCpuTurn()) return;
    if (cursorLastPos.x != cursorPos.x || cursorLastPos.y != cursorPos.y) {
        int dx = cursorLastPos.x * 12 + 16;
        memcpy(arduboy.getBuffer() + dx + cursorLastPos.y * WIDTH, cursorBuf, 11);
        cursorLastPos = cursorPos;
        isCursorMoved = true;
    }
    if (isCursorMoved) {
        int dx = cursorPos.x * 12 + 16;
        memcpy(cursorBuf, arduboy.getBuffer() + dx + cursorPos.y * WIDTH, 11);
        isCursorMoved = false;
    }
    bool isBlink = (gameFrames & 2);
    arduboy.drawRect(cursorPos.x * 12 + 16, cursorPos.y * 8 + 1, 11, 7, isBlink ? WHITE : BLACK);
}

static void drawPlayerIndication(bool isAnimation)
{
    if (isAnimation) {
        if (gameFrames & 7) return;
        arduboy.fillRect2(0, 24, 11, 19, BLACK);
        arduboy.fillRect2(116, 24, 11, 19, BLACK);
    } else {
        drawStone(0, 1, IMG_ID_BLACK_OUTSIDE);
        drawStone(116, 1, IMG_ID_WHITE_OUTSIDE);
    }

    bool isActive = (board.numStones >= 4 && state != STATE_OVER);
    int a = gameFrames / 8 & 3;
    for (int i = 0; i < 2; i++) {
        bool isCpu = (1 - i == record.gameMode);
        int x = i * 116;
        int y = (isActive && i == board.isWhiteTurn) ? 26 + abs(a - 1) : 27;
        arduboy.drawBitmap(x, y, imgPlayer[isCpu], 11, 11, WHITE);
        if (isCpu) drawNumber(x + 3, y + 3, record.cpuLevel);
    }
    if (isActive) {
        arduboy.fillRect2(board.isWhiteTurn ? 116 : 0, 40, 11, 3, WHITE);
        if (board.numPlaceable == 0) {
            arduboy.fillRect2(50, 28, 27, 9, WHITE);
            arduboy.fillRect2(51, 29, 25, 7, BLACK);
            if (a & 1) arduboy.printEx(52, 30, F("PASS"));
        }
    }
}

static void drawStonesCounter(void)
{
    if ((record.settings & SETTING_BIT_STONES_COUNTER) || state == STATE_OVER) {
        draw2Numbers(0, 18, board.numBlack);
        draw2Numbers(116, 18, board.numWhite);
    }
}

static void draw2Numbers(int16_t x, int16_t y, int8_t value)
{
    arduboy.setCursor(x, y);
    if (value < 10) arduboy.print(' ');
    arduboy.print(value);
}


static void drawResult(void)
{
    arduboy.fillRect2(35, 28, 57, 9, WHITE);
    arduboy.fillRect2(36, 29, 55, 7, BLACK);
    arduboy.printEx(37, 30, F("GAME OVER"));
}

/*---------------------------------------------------------------------------*/
/*                             Board Management                              */
/*---------------------------------------------------------------------------*/

static void analyzeBoard(BOARD_T *p)
{
    p->numBlack = 0;
    p->numWhite = 0;
    p->numPlaceable = 0;
    for (int8_t y = 0; y < BOARD_H; y++) {
        uint8_t black = p->black[y];
        uint8_t white = p->white[y];
        uint8_t flag = p->flag[y];
        p->numBlack += countBits(black);
        p->numWhite += countBits(white);
        for (int8_t x = 0; x < BOARD_W; x++) {
            uint8_t b = 1 << x;
            if (~(black | white) & b) {
                if (isPlaceable(p, x, y, false)) {
                    flag |= b;
                } else {
                    flag &= ~b;
                }
            }
        }
        p->flag[y] = flag;
        p->numPlaceable += countBits(~(black | white) & flag);
    }
    p->numStones = p->numBlack + p->numWhite;
    checkFixedStones(p);
}

static bool isPlaceable(BOARD_T *p, int8_t x, int8_t y, bool isActual)
{
    bool ret = false;
    if (isActual) {
        uint8_t b = 1 << x;
        if (p->isWhiteTurn) {
            p->white[y] |= b;
        } else {
            p->black[y] |= b;
        }
        p->flag[y] &= ~b;
        (p->isWhiteTurn) ? p->numWhite++ : p->numBlack++;
    }
    for (int8_t vy = -1; vy <= 1; vy++) {
        for (int8_t vx = -1; vx <= 1; vx++) {
            if (vx == 0 && vy == 0) continue;
            if (isReversible(p, x, y, vx, vy, isActual)) {
                if (!isActual) return true;
                ret = true;
            }
        }
    }
    return ret;
}

static bool isReversible(BOARD_T *p, int8_t x, int8_t y, int8_t vx, int8_t vy, bool isActual)
{
    for (int s = 0; ; s++) {
        x += vx;
        y += vy;
        if (x < 0 || y < 0 || x >= BOARD_W || y >= BOARD_H) return false;
        uint8_t b = 1 << x;
        bool isBlack = p->black[y] & b;
        bool isWhite = p->white[y] & b;
        if (!isBlack && !isWhite) return false;
        if (isWhite == p->isWhiteTurn) {
            if (s == 0) return false;
            if (isActual) {
                animStones += s;
                int8_t anim = animStones * 4 + 32;
                while (s-- > 0) {
                    x -= vx;
                    y -= vy;
                    b = 1 << x;
                    p->black[y] ^= b;
                    p->white[y] ^= b;
                    animTable[y][x] = anim;
                    anim -= 4;
                }
            }
            return true;
        }
    }
}

static void checkFixedStones(BOARD_T *p)
{
    bool isUpdated;
    do {
        p->numFixedBlack = 0;
        p->numFixedWhite = 0;
        isUpdated = false;
        for (int8_t y = 0; y < BOARD_H; y++) {
            uint8_t black = p->black[y];
            uint8_t white = p->white[y];
            uint8_t stones = black | white;
            if (stones == 0) continue;
            uint8_t flag = p->flag[y];
            for (int8_t x = 0; x < BOARD_W; x++) {
                uint8_t b = 1 << x;
                if (stones & b) {
                    bool isCheckingBlack = black & b;
                    if ((~flag & b) && isFixed(p, x, y, isCheckingBlack)) {
                        flag |= b;
                        isUpdated = true;
                    }
                }
            }
            p->flag[y] = flag;
            p->numFixedBlack += countBits(black & flag);
            p->numFixedWhite += countBits(white & flag);
        }
    } while (isUpdated);
}

static bool isFixed(BOARD_T *p, int8_t x, int8_t y, bool isCheckingBlack)
{
    for (int8_t vy = -1; vy <= 0; vy++) {
        int8_t vxMax = (vy == -1) ? 1 : -1;
        for (int8_t vx = -1; vx <= vxMax; vx++) {
            FIX_COND_T cond1 = checkFixCond(p, x, y, vx, vy, isCheckingBlack);
            FIX_COND_T cond2 = checkFixCond(p, x, y, -vx, -vy, isCheckingBlack);
            if (cond1 == EXIST_EMPTY && cond2 == EXIST_EMPTY ||
                    cond1 == EXIST_EMPTY && cond2 == NO_EMPTY ||
                    cond1 == NO_EMPTY && cond2 == EXIST_EMPTY) return false;
        }
    }
    return true;
}

static FIX_COND_T checkFixCond(BOARD_T *p, int8_t x, int8_t y, int8_t vx, int8_t vy, bool isCheckingBlack)
{
    FIX_COND_T ret = NEIGHBOR_FIXED;
    while (true) {
        x += vx;
        y += vy;
        if (x < 0 || y < 0 || x >= BOARD_W || y >= BOARD_H) return ret;
        uint8_t b = 1 << x;
        bool isBlack = p->black[y] & b;
        bool isWhite = p->white[y] & b;
        if (!isBlack && !isWhite) return EXIST_EMPTY;
        if (ret == NEIGHBOR_FIXED) {
            if ((p->flag[y] & b) && isBlack == isCheckingBlack) return NEIGHBOR_FIXED;
            ret = NO_EMPTY;
        }
    }
}

static bool isGameOver(BOARD_T *p)
{
    return  p->numStones == BOARD_H * BOARD_W ||
            p->numBlack == 0 ||
            p->numWhite == 0 ||
            p->isLastPassed && p->numPlaceable == 0;

}

/*---------------------------------------------------------------------------*/
/*                            Thinking Algorithm                             */
/*---------------------------------------------------------------------------*/

static void cpuThinking(void)
{
    int8_t depth = record.cpuLevel;
    if (board.numPlaceable == 1) depth = 1;
    if (board.numStones < 16) depth = min(depth, board.numStones / 4 + 1);
    alphabeta(NULL, depth, -EVAL_INF, EVAL_INF);
    arduboy.setRGBled(0, 0, 0);
}

static int alphabeta(BOARD_T *p, int8_t depth, int alpha, int beta)
{
    bool isRoot;
    if (p == NULL) {
        p = &board;
        isRoot = true;
        thinkLed = 0;
    } else {
        analyzeBoard(p);
        isRoot = false;
    }

    if (depth-- <= 0 || isGameOver(p)) {
        return -evaluateBoard(p) + random(3);
    }
    if (p->numPlaceable == 0) {
        BOARD_T tmpBoard = *p;
        tmpBoard.isWhiteTurn = !tmpBoard.isWhiteTurn;
        tmpBoard.isLastPassed = true;
        return -alphabeta(&tmpBoard, depth, -beta, -alpha);
    }
    for (int8_t y = 0; y < BOARD_H; y++) {
        uint8_t black = p->black[y];
        uint8_t white = p->white[y];
        uint8_t flag = p->flag[y];
        uint8_t placeable = ~(black | white) & flag;
        if (placeable == 0) continue;
        for (int8_t x = 0; x < BOARD_W; x++) {
            uint8_t b = 1 << x;
            if (placeable & b) {
                if ((record.settings & SETTING_BIT_THINK_LED) && (thinkLed & 0x3f) == 0) {
                    uint8_t r = thinkLed >> 3;
                    arduboy.setRGBled((r < 64) ? r : 128 - r, 0, 0);
                }
                if (++thinkLed >= 1024) thinkLed = 0;
                BOARD_T tmpBoard = *p;
                placeStone(&tmpBoard, x, y);
                tmpBoard.isWhiteTurn = !tmpBoard.isWhiteTurn;
                tmpBoard.isLastPassed = false;
                int eval = alphabeta(&tmpBoard, depth, -beta, -alpha);
                if (eval > alpha) {
                    alpha = eval;
                    if (isRoot) {
                        cursorPos.x = x;
                        cursorPos.y = y;
                    }
                }
                if (alpha >= beta) return -alpha;
            }
        }
    }
    return -alpha;
}

static int evaluateBoard(BOARD_T *p)
{
    int eval = 0;
    if (isGameOver(p)) {
        if (p->numBlack != p->numWhite) {
            eval = (p->numBlack > p->numWhite) ? EVAL_WIN : EVAL_LOSE;
        }
    } else {
        for (int8_t y = 0; y < BOARD_H; y++) {
            uint8_t black = p->black[y];
            uint8_t white = p->white[y];
            uint8_t flag = p->flag[y];
            int8_t row = (y < BOARD_H / 2) ? y : BOARD_H - 1 - y;
            const int8_t *pTable = evalStonesTable[row];
            eval += evaluateBit(pTable, black & ~flag);
            eval -= evaluateBit(pTable, white & ~flag);
            pTable = evalFixedStonesTable[row];
            eval += evaluateBit(pTable, black & flag);
            eval -= evaluateBit(pTable, white & flag);
        }
    }
    if (p->isWhiteTurn) eval = -eval;
    eval += p->numPlaceable;
    if (p->numPlaceable == 0) eval += EVAL_NOPLACEABLE;
    return eval;
}

static int8_t evaluateBit(const int8_t *pTable, uint8_t value)
{
    return  (int8_t) pgm_read_byte(pTable + (value & 0xf)) +
            (int8_t) pgm_read_byte(pTable + 16 + (value >> 4 & 0xf));
}
