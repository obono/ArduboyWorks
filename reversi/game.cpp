#include "common.h"
#include "data.h"

/*  Defines  */

#define BOARD_W 8
#define BOARD_H 8
#define INITIAL_STONES 4

enum STATE_T {
    STATE_INIT = 0,
    STATE_PLAYING,
    STATE_FLIPPING,
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
    uint8_t black[BOARD_H], white[BOARD_H], flag[BOARD_H];
    uint8_t numStones, numBlack, numWhite, numFixedBlack, numFixedWhite, numPlaceable;
    bool    isWhiteTurn, isLastPassed;
} BOARD_T;

/*  Local Functions  */

static void newGame(void);
static void resumeGame(void);
static void finalizeGame(void);
static void toggleTurn(bool isPassed);
static bool isCpuTurn(void);
static bool isGameOver(BOARD_T *p);
static void resetFlipAnimationParams(void);
static void handlePlaying(void);
static void handleFlipping(void);
static void handleOver(void);
static void onContinue(void);
static void onCancel(void);
static void onSuspend(void);
static void onRetry(void);
static void onExit(void);

static void drawBoard(bool isAnimation);
static void drawStone(int8_t x, int8_t y, int8_t p);
static void drawFlippingStone(int8_t x, int8_t y, int8_t anim, bool isBlack);
static void drawCursor(void);
static void drawPlayerIndication(bool isAnimation);
static void drawStonesCounter(void);
static void draw2Numbers(int16_t x, int16_t y, int8_t value);
static void drawResult(void);
#define     getDx(x) ((x) * 12 + 16)
#define     getDy(y) ((y) * 8)

static void analyzeBoard(BOARD_T *p);
static void placeStone(void);
static bool isPlaceable(BOARD_T *p, int8_t x, int8_t y, bool isActual);
static bool isReversible(BOARD_T *p, int8_t x, int8_t y, int8_t vx, int8_t vy, bool isActual);
static void checkFixedStones(BOARD_T *p);
static bool isFixed(BOARD_T *p, int8_t x, int8_t y, bool isCheckingBlack);
static FIX_COND_T checkFixCond(BOARD_T *p, int8_t x, int8_t y, int8_t vx, int8_t vy, bool isCheckingBlack);
#define     countBits(val) pgm_read_byte(bitNumTable + (val))

static void cpuThinking(void);
static int  alphabeta(BOARD_T *p, int8_t depth, int alpha, int beta);
static int  evaluateBoard(BOARD_T *p);
static int8_t evaluateBit(const int8_t *pTable, uint8_t value);

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static BOARD_T  board, lastStepBoard;
static POS_T    cursorPos, cursorLastPos, lastStepPos;
static uint16_t thinkLed;
static uint8_t  cursorBuf[IMG_STONE_W];
static uint8_t  ledRGB[3];
static int8_t   flipTable[BOARD_H][BOARD_W];
static uint8_t  flipStones, animCounter, counter;
static bool     isCursorMoved, isCancelable;
static const byte *resultSound;
static const __FlashStringHelper *resultLabel;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    if (record.canContinue) {
        resumeGame();
    } else {
        newGame();
    }
    isRecordDirty = true;
    writeRecord();

    resetFlipAnimationParams();
    isCancelable = false;
    arduboy.playScore2(soundStart, 0);
    isInvalid = true;
}

MODE_T updateGame(void)
{
    counter++;
    if (state == STATE_PLAYING || state == STATE_FLIPPING) {
        record.playFrames++;
        isRecordDirty = true;
    }
    switch (state) {
    case STATE_PLAYING:
        handlePlaying();
        break;
    case STATE_FLIPPING:
        handleFlipping();
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
        case STATE_FLIPPING:
            drawBoard(true);
            drawPlayerIndication(true);
            if (!(animCounter & 3)) drawStonesCounter();
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

static void newGame(void)
{
    record.playCount++;
    memset(&board, 0, sizeof(board));
    board.isWhiteTurn = true; // trick
    state = STATE_FLIPPING;
    dprintln(F("New game"));
}

static void resumeGame(void)
{
    memcpy(board.black, record.black, BOARD_H * 2);
    board.isWhiteTurn = record.isWhiteTurn;
    board.isLastPassed = record.isLastPassed;
    cursorPos = record.cursorPos;
    record.canContinue = false;
    analyzeBoard(&board);
    state = STATE_PLAYING;
    dprintln(F("Resume game"));
}

static void finalizeGame(void)
{
    bool isBlackWin = board.numBlack > board.numWhite;
    if (board.numBlack == board.numWhite) {
        resultSound = soundDraw;
        resultLabel = F("DRAW");
    } else if (record.gameMode == GAME_MODE_2PLAYERS) {
        resultSound = soundWin;
        resultLabel = (isBlackWin) ? F("BLACK WINS") : F("WHITE WINS");
    } else {
        bool isPlayerWin = record.gameMode == GAME_MODE_BLACK && isBlackWin ||
                record.gameMode == GAME_MODE_WHITE && !isBlackWin;
        resultSound = (isPlayerWin) ? soundWin : soundLose;
        resultLabel = (isPlayerWin) ? F("YOU WIN") : F("YOU LOSE");
        if (record.cpuLevel == 3 + record.isAdvancedLevel &&
                record.vsCpuWinLose != WIN_LOSE_VALUE_MAX) {
            int win = record.vsCpuWinLose / (WIN_LOSE_MAX + 1);
            int lose = record.vsCpuWinLose % (WIN_LOSE_MAX + 1);
            if (isPlayerWin && win < WIN_LOSE_MAX) win++;
            if (!isPlayerWin && lose < WIN_LOSE_MAX) lose++;
            dprint(F("Vs CPU level="));
            dprint(record.cpuLevel);
            dprint(F(" : Win "));
            dprint(win);
            dprint(F(" - Lose "));
            dprintln(lose);
            if (win > lose || win == WIN_LOSE_MAX) {
                if (record.isAdvancedLevel) {
                    record.vsCpuWinLose = WIN_LOSE_VALUE_MAX;
                } else {
                    record.isAdvancedLevel = true;
                    record.vsCpuWinLose = 0;
                }
                dprintln(F("Level up!!!"));
            } else {
                record.vsCpuWinLose = win * (WIN_LOSE_MAX + 1) + lose;
            }
        }
    }
    writeRecord();
    animCounter = 0;
    state = STATE_OVER;
    dprint(F("Game set: "));
    dprintln(resultLabel);
}

static void toggleTurn(bool isPassed)
{
    bool isDoublePass = isPassed && board.isLastPassed;
    board.isWhiteTurn = !board.isWhiteTurn;
    board.isLastPassed = isPassed;
    analyzeBoard(&board);
    resetFlipAnimationParams();
    int numStones = board.numStones;
    if (numStones < INITIAL_STONES) {
        cursorPos.x = (numStones == 0 || numStones == 3) ? 3 : 4;
        cursorPos.y = (numStones == 0 || numStones == 1) ? 3 : 4;
        placeStone();
    } else if (isDoublePass || isGameOver(&board)) {
        finalizeGame();
    } else {
        if (board.numPlaceable == 0) arduboy.playScore2(soundPass, SND_PRIO_PASS);
        state = STATE_PLAYING;
    }
    isInvalid = true;
}

static bool isCpuTurn(void)
{
    return  board.isWhiteTurn && record.gameMode == GAME_MODE_BLACK ||
            !board.isWhiteTurn && record.gameMode == GAME_MODE_WHITE;
}

static bool isGameOver(BOARD_T *p)
{
    return  p->numStones == BOARD_H * BOARD_W ||
            p->numBlack == 0 ||
            p->numWhite == 0 ||
            p->isLastPassed && p->numPlaceable == 0;

}

static void resetFlipAnimationParams(void)
{
    memset(flipTable, 0, sizeof(flipTable));
    flipStones = 0;
    animCounter = 0;
    cursorLastPos = cursorPos;
    isCursorMoved = true;
}

static void handlePlaying(void)
{
    if (board.numPlaceable == 0) {
        /*  Pass  */
        bool isCpu = isCpuTurn();
        if (isCpu && ++animCounter >= FPS || !isCpu && arduboy.buttonDown(B_BUTTON)) {
            if (!isCpu) playSoundClick();
            toggleTurn(true);
        }
    } else if (isCpuTurn()) {
        /*  CPU thinking  */
        cpuThinking();
        resetFlipAnimationParams();
        placeStone();
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
                lastStepBoard = board;
                lastStepPos = cursorPos;
                isCancelable = true;
                placeStone();
            }
        }
    }

    if (state == STATE_PLAYING && arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("CONTINUE"), onContinue);
        if (isCancelable) addMenuItem(F("CANCEL LAST MOVE"), onCancel);
        addMenuItem(F("SUSPEND GAME"), onSuspend);
        addMenuItem(F("ABANDON GAME"), onExit);
        int w = (isCancelable) ? 107 : 83; 
        int h = (isCancelable) ? 23 : 17;
        setMenuCoords(63 - w / 2, 32 - h / 2, w, h, true, true);
        setMenuItemPos(0);
        state = STATE_MENU;
        isInvalid = true;
        dprintln(F("Menu: playing"));
    }

#ifdef DEBUG
    if (dbgRecvChar == 'p') {
        placeStone();
    }
    if (dbgRecvChar == 'q') {
        finalizeGame();
    }
#endif
}

static void handleFlipping(void)
{
    uint8_t animCounterMax = (flipStones > 0) ? flipStones * 4 + 32 : 20;
    if (++animCounter < animCounterMax) {
        if (!(animCounter & 3) && animCounter >= 20 && animCounter < flipStones * 4 + 20) {
            if (board.isWhiteTurn) {
                board.numBlack--;
                board.numWhite++;
            } else {
                board.numBlack++;
                board.numWhite--;
            }
            arduboy.playScore2(soundFlip, SND_PRIO_FLIP);
        }
    } else {
        toggleTurn(false);
    }
}

static void handleOver(void)
{
    if (animCounter < FPS * 3) {
        animCounter++;
        if (animCounter == FPS) arduboy.playScore2(resultSound, SND_PRIO_RESULT);
        if (animCounter == FPS * 3) {
            arduboy.stopScore2();
            if (arduboy.buttonPressed(B_BUTTON)) isInvalid = true;
        }
    } else {
        if (arduboy.buttonDown(B_BUTTON)) isInvalid = true;
        if (arduboy.buttonUp(B_BUTTON)) animCounter = FPS;
        if (arduboy.buttonDown(A_BUTTON)) {
            playSoundClick();
            clearMenuItems();
            addMenuItem(F("RETRY GAME"), onRetry);
            addMenuItem(F("BACK TO TITLE"), onExit);
            setMenuCoords(19, 19, 89, 11, true, false);
            setMenuItemPos(0);
            state = STATE_MENU;
            isInvalid = true;
            dprintln(F("Menu: game over"));
        }
    }
}

static void onContinue(void)
{
    playSoundClick();
    state = STATE_PLAYING;
    isInvalid = true;
}

static void onCancel(void)
{
    board = lastStepBoard;
    cursorPos = lastStepPos;
    cursorLastPos = cursorPos;
    isCursorMoved = true;
    isCancelable = false;
    arduboy.playScore2(soundCancel, SND_PRIO_CANCEL);
    state = STATE_PLAYING;
    isInvalid = true;
}

static void onSuspend(void)
{
    playSoundClick();
    memcpy(record.black, board.black, BOARD_H * 2);
    record.isWhiteTurn = board.isWhiteTurn;
    record.isLastPassed = board.isLastPassed;
    record.cursorPos = cursorPos;
    record.canContinue = true;
    writeRecord();
    state = STATE_LEAVE;
    dprintln(F("Suspend game"));
}

static void onRetry(void)
{
    initGame();
}

static void onExit(void)
{
    playSoundClick();
    writeRecord();
    state = STATE_LEAVE;
    dprintln(F("Back to title"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawBoard(bool isAnimation)
{
    int anim = -1;
    for (int8_t y = 0; y < BOARD_H; y++) {
        uint8_t black = board.black[y];
        uint8_t white = board.white[y];
        uint8_t flag = board.flag[y];
        uint8_t placeable = flag & ~(black | white);
        uint8_t fixed = flag & (black | white);
        if (!(record.settings & SETTING_BIT_SHOW_PLACEABLE) || state != STATE_PLAYING) placeable = 0;
        if (!(record.settings & SETTING_BIT_SHOW_FIXED)) fixed = 0;
        int dy = getDy(y);
        for (int8_t x = 0; x < BOARD_W; x++) {
            int dx = getDx(x);
            uint8_t b = 1 << x;
            if (isAnimation) {
                anim = flipTable[y][x] - animCounter;
                int belowAnim = (y < BOARD_H - 1) ? flipTable[y + 1][x] - animCounter : -1;
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
    memcpy_P(arduboy.getBuffer() + dx + y * WIDTH, imgStone[p], IMG_STONE_W);
}

static void drawFlippingStone(int8_t dx, int8_t dy, int8_t anim, bool isBlack) {
    arduboy.drawBitmap(dx, dy - 8, imgFlippingBase[anim], IMG_FSTONE_W, IMG_FSTONE_H, WHITE);
    if (anim > 4) isBlack = !isBlack;
    if (isBlack) {
        arduboy.drawBitmap(dx, dy - 8, imgFlippingFace[anim], IMG_FSTONE_W, IMG_FSTONE_H, BLACK);
    }
}

static void drawCursor(void) {
    if (board.numStones < INITIAL_STONES || board.numPlaceable == 0 || isCpuTurn()) return;
    if (cursorLastPos.x != cursorPos.x || cursorLastPos.y != cursorPos.y) {
        int dx = getDx(cursorLastPos.x);
        memcpy(arduboy.getBuffer() + dx + cursorLastPos.y * WIDTH, cursorBuf, IMG_STONE_W);
        cursorLastPos = cursorPos;
        isCursorMoved = true;
    }
    int dx = getDx(cursorPos.x);
    int dy = getDy(cursorPos.y) + 1;
    if (isCursorMoved) {
        memcpy(cursorBuf, arduboy.getBuffer() + dx + cursorPos.y * WIDTH, IMG_STONE_W);
        isCursorMoved = false;
    }
    arduboy.drawRect(dx, dy, IMG_STONE_W, IMG_STONE_H - 1, (counter & 2) ? WHITE : BLACK);
}

static void drawPlayerIndication(bool isAnimation)
{
    if (isAnimation) {
        arduboy.fillRect2(0, 24, IMG_PLAYER_W, IMG_PLAYER_H + 8, BLACK);
        arduboy.fillRect2(116, 24, IMG_PLAYER_W, IMG_PLAYER_H + 8, BLACK);
    } else {
        drawStone(0, 1, IMG_ID_BLACK_OUTSIDE);
        drawStone(116, 1, IMG_ID_WHITE_OUTSIDE);
    }

    bool isActive = (board.numStones >= INITIAL_STONES && state != STATE_OVER);
    int a = counter / 8 & 3;
    for (int i = 0; i < 2; i++) {
        bool isCpu = (1 - i == record.gameMode);
        int x = i * 116;
        int y = (isActive && i == board.isWhiteTurn) ? 26 + abs(a - 1) : 27;
        arduboy.drawBitmap(x, y, imgPlayer[isCpu], IMG_PLAYER_W, IMG_PLAYER_H, WHITE);
        if (isCpu) drawNumber(x + 3, y + 3, record.cpuLevel);
    }
    if (isActive) {
        arduboy.fillRect2(board.isWhiteTurn ? 116 : 0, 40, IMG_PLAYER_W, 3, WHITE);
        if (board.numPlaceable == 0) {
            int x = board.isWhiteTurn ? 82 : 22;
            arduboy.fillRect2(x - 2, 28, 27, 9, WHITE);
            arduboy.fillRect2(x - 1, 29, 25, 7, BLACK);
            if (a & 1) arduboy.printEx(x, 30, F("PASS"));
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
    if (animCounter < FPS || animCounter > FPS * 2) return;
    int h = min(animCounter - FPS + 1, 12);
    arduboy.fillRect2(0, 51 - h / 2, WIDTH, h + 2, BLACK);    
    arduboy.fillRect2(0, 52 - h / 2, WIDTH, h, WHITE);
    int a = FPS * 2 - animCounter;
    int x = a * a / 18 + WIDTH / 2 - (strlen_PF((uint_farptr_t) resultLabel) * 6 - 1);
    if (x < WIDTH) {
        arduboy.setTextColor(BLACK, BLACK);
        arduboy.setTextSize(2);
        arduboy.printEx(x, 47, resultLabel);
        arduboy.setTextColor(WHITE, BLACK);
        arduboy.setTextSize(1);
    }
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
    dprintln(F("Board analysis"));
    dprintln(board.numBlack);
    dprintln(board.numWhite);
    dprintln(board.numFixedBlack);
    dprintln(board.numFixedWhite);
    dprintln(board.numPlaceable);
}

static void placeStone(void)
{
    isPlaceable(&board, cursorPos.x, cursorPos.y, true);
    arduboy.playScore2(soundPlace, SND_PRIO_PLACE);
    state = STATE_FLIPPING;
    dprint(F("Place "));
    dprint((board.isWhiteTurn) ? F("White ") : F("Black "));
    dprint(cursorPos.x);
    dprint(',');
    dprintln(cursorPos.y);
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
                flipStones += s;
                int8_t anim = flipStones * 4 + 32;
                while (s-- > 0) {
                    x -= vx;
                    y -= vy;
                    b = 1 << x;
                    p->black[y] ^= b;
                    p->white[y] ^= b;
                    flipTable[y][x] = anim;
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

/*---------------------------------------------------------------------------*/
/*                            Thinking Algorithm                             */
/*---------------------------------------------------------------------------*/

static void cpuThinking(void)
{
    int8_t depth = min(record.cpuLevel, board.numStones / 4 + 1);
    if (board.numPlaceable == 1) depth = 1;
    unsigned long before = millis();
    int eval = -alphabeta(NULL, depth, -EVAL_INF, EVAL_INF);
    arduboy.setRGBled(0, 0, 0);
    unsigned long after = millis();
    if (after > before) {
        int diff = (after - before) * FPS / 1000;
        record.playFrames += diff; // TODO
        dprint(F("CPU thinking frames="));
        dprintln(diff);
    }
    dprint(F("Board evaluation="));
    dprintln(eval);
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
                if ((record.settings & SETTING_BIT_THINK_LED) && !(thinkLed & 0x3f)) {
                    uint8_t r = thinkLed >> 3;
                    arduboy.setRGBled((r < 64) ? r : 128 - r, 0, 0);
                }
                if (++thinkLed >= 1024) thinkLed = 0;
                BOARD_T tmpBoard = *p;
                isPlaceable(&tmpBoard, x, y, true);
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
