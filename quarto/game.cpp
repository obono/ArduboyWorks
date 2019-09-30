#include "common.h"
#include "data.h"

/*  Defines  */

#define ANIM_COUNTER_MOVE   16
#define ANIM_RESOLUTION     (ANIM_COUNTER_MOVE * ANIM_COUNTER_MOVE)

#define CPU_INTERVAL_FRAMES 8
#define CPU_INTERVAL_MILLIS ((1000 * CPU_INTERVAL_FRAMES) / FPS)

#define EVAL_INF    32767
#define EVAL_WIN    5400

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))

enum STATE_T {
    STATE_INIT = 0,
    STATE_PRE_PLACE,
    STATE_PLACING,
    STATE_POST_PLACE,
    STATE_CHOOSING,
    STATE_MENU,
    STATE_OVER,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    uint8_t     board[BOARD_SIZE][BOARD_SIZE];
    uint8_t     turn;
    uint8_t     currentPiece;
    uint16_t    restPieces;
} GAME_T;

/*  Local Functions  */

static bool isCpuTurn(void);
static bool isShowHint(void);
static void handleMovingPiece(void);
static void handlePlacing(void);
static void handleChoosing(void);
static void handleOver(void);
static void calcRestPiecesLeft(void);

static void newGame(void);
static void placePiece(void);
static void choosePiece(void);
static void finalizeGame(void);
static int  countWinMoves(GAME_T *p, bool isCpu, bool isHint);
static bool isWinMove(GAME_T *p, uint8_t x, uint8_t y, bool isJudge);
static bool isLined(GAME_T *p, uint8_t x, uint8_t y, int8_t vx, int8_t vy, bool isJudge);

static void onContinue(void);
static void onCancel(void);
static void onRetry(void);
static void onQuit(void);
static void onExit(void);

static void drawBoard(bool isFullUpdate);
static void drawBoardEdge(int16_t x);
static void drawBoardUnit(uint8_t x, uint8_t y, bool isDrawHint, int8_t shake);
static void drawPlayers(bool isFullUpdate);
static void drawMovingPiece(void);
static void drawRestPieces(bool isFullUpdate);
static void drawRestPiecesUnit(int16_t x, uint8_t piece, bool isDrawHint);
static void drawResult(bool isFullUpdate);
//     void drawPiece(int16_t x, int16_t y, int16_t piece);
static void drawCursor(int16_t x, int16_t y);

static bool cpuThinking(void);
static void cpuThinkingInterval(void);
static int  alphabeta(GAME_T *p, int8_t depth, int alpha, int beta);

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static GAME_T   game, lastGame;
static bool     isCancelable, isCpuInterrupted, isLastAPressed, isHideOverlay;
static uint8_t  counter, animCounter;
static uint8_t  cursorX, cursorY, cursorPiece;
static uint8_t  cpuX, cpuY, cpuPiece;
static int8_t   animSx, animSy, animDx, animDy, restPiecesLeft;
static uint16_t hintWinMoves, hintNgPieces, linedPiecesPos;
static unsigned long nextCpuInterval;
static const byte *resultSound;
static const __FlashStringHelper *resultLabel;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    arduboy.playScore2(soundStart, 0);
    record.playCount++;
    isRecordDirty = true;
    writeRecord();
    newGame();
}

MODE_T updateGame(void)
{
    counter++;
    if (state >= STATE_PRE_PLACE || state <= STATE_CHOOSING) {
        record.playFrames++;
        isRecordDirty = true;
    }
    switch (state) {
    case STATE_PRE_PLACE:
    case STATE_POST_PLACE:
        handleMovingPiece();
        break;
    case STATE_PLACING:
        handlePlacing();
        break;
    case STATE_CHOOSING:
        handleChoosing();
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
        drawBoard(true);
    }
    drawPlayers(isInvalid);
    switch (state) {
    case STATE_PRE_PLACE:
    case STATE_POST_PLACE:
        drawMovingPiece();
        break;
    case STATE_PLACING:
        if (!isInvalid) drawBoard(false);
        break;
    case STATE_CHOOSING:
        if (!isHideOverlay) drawRestPieces(isInvalid);
        break;
    case STATE_OVER:
        if (!isHideOverlay) drawResult(isInvalid);
        break;
    }
    isInvalid = false;
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static bool isCpuTurn(void)
{
    return (record.gameMode != GAME_MODE_2PLAYERS &&
            game.turn < TURN_MAX &&
            game.turn % 2 != record.gameMode);
}

static bool isShowHint(void)
{
    return ((record.settings & SETTING_BIT_HINT) &&
            record.gameMode != GAME_MODE_2PLAYERS &&
            game.turn < TURN_MAX &&
            game.turn % 2 == record.gameMode);
}

static void handleMovingPiece(void)
{
    isInvalid = true;
    if (--animCounter > 0) return;
    arduboy.playScore2(soundPlace, SND_PRIO_PLACE);
    switch (state) {
    case STATE_PRE_PLACE:
        state = STATE_PLACING;
        break;
    case STATE_POST_PLACE:
        placePiece();
        if (state == STATE_CHOOSING && isCpuTurn()) choosePiece();
        break;
    }
}

static void handlePlacing(void)
{
    bool isSettled = false;
    bool isMenuOpened = arduboy.buttonDown(A_BUTTON);

    if (isCpuTurn()) {
        if (!isMenuOpened) {
            if (cpuThinking()) {
                cursorX = cpuX;
                cursorY = cpuY;
                cursorPiece = cpuPiece;
                isSettled = true;
            } else {
                isMenuOpened = true;
            }
        }
    } else {
        handleDPad();
        if (padX != 0 || padY != 0) {
            /*  Move cursor  */
            playSoundTick();
            cursorX = circulate(cursorX, padX, BOARD_SIZE);
            cursorY = circulate(cursorY, padY, BOARD_SIZE);
        }
        isSettled = (arduboy.buttonDown(B_BUTTON) && game.board[cursorY][cursorX] == BOARD_EMPTY);
    }

    if (isSettled) {
        /*  Start animcation  */
        animSx = game.turn % 2 * 96 + 8;
        animSy = 40;
        animDx = cursorX * 16 + 32;
        animDy = cursorY * 16;
        animCounter = ANIM_COUNTER_MOVE;
        state = STATE_POST_PLACE;
        isInvalid = true;
    } else if (isMenuOpened) {
        playSoundClick();
        writeRecord();
        clearMenuItems();
        addMenuItem(F("CONTINUE"), onContinue);
        if (isCancelable) addMenuItem(F("CANCEL LAST MOVE"), onCancel);
        addMenuItem(F("QUIT GAME"), onQuit);
        int w = (isCancelable) ? 107 : 83; 
        int h = (isCancelable) ? 17 : 11;
        setMenuCoords(63 - w / 2, 32 - h / 2, w, h, true, true);
        setMenuItemPos(0);

        state = STATE_MENU;
        isInvalid = true;
        dprintln(F("Menu: playing"));
    }

#ifdef DEBUG
    if (dbgRecvChar == 'q') {
        finalizeGame();
    }
#endif
}

static void handleChoosing(void)
{
    bool isHideButtonPressed = arduboy.buttonPressed(UP_BUTTON | DOWN_BUTTON);
    if (arduboy.buttonDown(A_BUTTON)) {
        /*  Cancel  */
        arduboy.playScore2(soundCancel, SND_PRIO_CANCEL);
        game.board[cursorY][cursorX] = BOARD_EMPTY;
        state = STATE_PLACING;
        isInvalid = true;
        dprintln(F("Cancel"));
    } else if (isHideOverlay) {
        if (!isHideButtonPressed) {
            isHideOverlay = false;
            isInvalid = true;
        }
    } else if (isHideButtonPressed) {
        isHideOverlay = true;
        isInvalid = true;
    } else {
        handleDPad();
        if (padX != 0) {
            /*  Move cursor  */
            playSoundTick();
            do {
                cursorPiece = circulate(cursorPiece, padX, PIECE_MAX);
            } while (!(game.restPieces & 1 << cursorPiece));
            calcRestPiecesLeft();
        }
        if (arduboy.buttonDown(B_BUTTON)) {
            playSoundClick();
            choosePiece();
        }
    }
}

static void handleOver(void)
{
    if (game.turn < TURN_MAX) isInvalid = true;
    if (animCounter > 0) {
        animCounter--;
        if (animCounter == FPS) arduboy.playScore2(resultSound, SND_PRIO_RESULT);
        if (animCounter == 0 && resultSound != soundLose) arduboy.stopScore2();
    } else {
        bool isHideButtonPressed = arduboy.buttonPressed(UP_BUTTON | DOWN_BUTTON | B_BUTTON);
        if (isHideOverlay) {
            if (!isHideButtonPressed) {
                isHideOverlay = false;
                animCounter = 1;
            }
        } else if (isHideButtonPressed) {
            isHideOverlay = true;
            isInvalid = true;
        } else if (arduboy.buttonDown(A_BUTTON)) {
            playSoundClick();
            clearMenuItems();
            addMenuItem(F("RETRY GAME"), onRetry);
            addMenuItem(F("BACK TO TITLE"), onExit);
            setMenuCoords(19, 18, 89, 11, true, false);
            setMenuItemPos(0);

            state = STATE_MENU;
            isInvalid = true;
            dprintln(F("Menu: game over"));
        }
    }
}

static void calcRestPiecesLeft(void)
{
    uint8_t piecesCount = TURN_MAX - 1 - game.turn;
    uint8_t offset = 0;
    for (uint8_t piece = 0; piece < cursorPiece; piece++) {
        if (game.restPieces & 1 << piece) offset++;
    }
    restPiecesLeft = 64 - piecesCount * 8;
    if (piecesCount > 8) {
        int16_t gap = (piecesCount - 8) * 16;
        restPiecesLeft += gap / 2 - gap * offset / (piecesCount - 1);
    }
}

/*---------------------------------------------------------------------------*/
/*                             Board Management                              */
/*---------------------------------------------------------------------------*/

static void newGame(void)
{
    /*  Reset board  */
    memset(&game.board, BOARD_EMPTY, BOARD_SIZE * BOARD_SIZE);
    cursorX = 0;
    cursorY = 0;
    cursorPiece = random(16);
    game.turn = 0;
    game.currentPiece = cursorPiece;
    game.restPieces = ~(1 << cursorPiece);
    isCancelable = false;
    hintWinMoves = 0;
    dprint(F("New game: piece = "));
    dprintln(cursorPiece);

    /*  Start animation  */
    animSx = 56;
    animSy = 64;
    animDx = 8;
    animDy = 40;
    animCounter = ANIM_COUNTER_MOVE;
    state = STATE_PRE_PLACE;
    isInvalid = true;
}

static void placePiece(void)
{
    game.board[cursorY][cursorX] = game.currentPiece;
    dprint(F("Turn "));
    dprint(game.turn + 1);
    dprint(F(" : Place at "));
    dprint(cursorX);
    dprint(F(","));
    dprintln(cursorY);

    if (isWinMove(&game, cursorX, cursorY, true)) {
        /*  Win  */
        finalizeGame();
    } else if (game.restPieces == 0) {
        /*  Draw  */
        game.turn++;
        finalizeGame();
    } else {
        /*  Check NG pieces  */
        hintNgPieces = 0;
        uint8_t tmpPiece = game.currentPiece;
        for (uint8_t piece = 0; piece < PIECE_MAX; piece++) {
            if (game.restPieces & 1 << piece) {
                game.currentPiece = piece;
                if (countWinMoves(&game, false, false)) {
                    hintNgPieces |= 1 << piece;
                }
            }
        }
        game.currentPiece = tmpPiece;

        /*  Adjust cursorPiece  */
        int8_t vx = 1;
        while (!(game.restPieces & 1 << cursorPiece)) {
            cursorPiece += vx;
            if (cursorPiece >= PIECE_MAX - 1) vx = -1;
        }
        calcRestPiecesLeft();
        isHideOverlay = false;

        state = STATE_CHOOSING;
        isInvalid = true;
    }
}

static void choosePiece(void)
{
    /*  Backup last game state  */
    if (!isCpuTurn()) {
        lastGame = game;
        lastGame.board[cursorY][cursorX] = BOARD_EMPTY;
        isCancelable = true;
    }

    /*  Toggle turn  */
    game.turn++;
    game.currentPiece = cursorPiece;
    game.restPieces &= ~(1 << cursorPiece);
    countWinMoves(&game, false, true);
    dprint(F("Next piece = "));
    dprintln(cursorPiece);

    /*  Start animation  */
    animSx = 104 - game.turn % 2 * 96;
    animSy = 20;
    animDx = game.turn % 2 * 96 + 8;
    animDy = 40;
    animCounter = ANIM_COUNTER_MOVE;
    state = STATE_PRE_PLACE;
    isInvalid = true;
}

static void finalizeGame(void)
{
    if (game.turn == TURN_MAX) {
        resultSound = soundDraw;
        resultLabel = F("DRAW");
    } else if (record.gameMode == GAME_MODE_2PLAYERS) {
        resultSound = soundWin;
        resultLabel = (game.turn % 2 == 0) ? F("1P WINS") : F("2P WINS");
    } else {
        bool isPlayerWin = (game.turn % 2 == record.gameMode);
        resultSound = (isPlayerWin) ? soundWin : soundLose;
        resultLabel = (isPlayerWin) ? F("YOU WIN") : F("YOU LOSE");
        if (isPlayerWin && record.maxLevel < 5 && record.cpuLevel == record.maxLevel) {
            record.maxLevel++;
            dprintln(F("Level up!!!"));
        }
    }
    isHideOverlay = false;
    writeRecord();

    state = STATE_OVER;
    animCounter = FPS * 2;
    isInvalid = true;
    dprint(F("Game set: "));
    dprintln(resultLabel);
}

static int countWinMoves(GAME_T *p, bool isCpu, bool isHint) {
    int winMovesCount = 0;
    if (isHint) hintWinMoves = 0;
    for (uint8_t y = 0; y < BOARD_SIZE; y++) {
        for (uint8_t x = 0; x < BOARD_SIZE; x++) {
            if (p->board[y][x] == BOARD_EMPTY) {
                if (isCpu && p->turn >= TURN_MAX - 1) {
                    cpuX = x;
                    cpuY = y;
                }
                p->board[y][x] = p->currentPiece;
                if (isWinMove(p, x, y, false)) {
                    winMovesCount++;
                    if (isCpu) {
                        cpuX = x;
                        cpuY = y;
                    }
                    if (isHint) {
                        hintWinMoves |= 1 << (y * BOARD_SIZE + x);
                    }
                }
                p->board[y][x] = BOARD_EMPTY;
            }
        }
    }
    return winMovesCount;
}

static bool isWinMove(GAME_T *p, uint8_t x, uint8_t y, bool isJudge)
{
    if (isJudge) linedPiecesPos = 0;
    bool ret = isLined(p, x, 0, 0, 1, isJudge) || isLined(p, 0, y, 1, 0, isJudge);
    if (!ret && x == y) ret = isLined(p, 0, 0, 1, 1, isJudge);
    if (!ret && x == BOARD_SIZE - y - 1) ret = isLined(p, BOARD_SIZE - 1, 0, -1, 1, isJudge);
    if (record.settings & SETTING_BIT_2x2_RULE) {
        if (!ret && x > 0 && y > 0) ret = isLined(p, x - 1, y - 1, 0, 0, isJudge);
        if (!ret && x < BOARD_SIZE - 1 && y > 0) ret = isLined(p, x, y - 1, 0, 0, isJudge);
        if (!ret && x > 0 && y < BOARD_SIZE - 1) ret = isLined(p, x - 1, y, 0, 0, isJudge);
        if (!ret && x < BOARD_SIZE - 1 && y < BOARD_SIZE - 1) ret = isLined(p, x, y, 0, 0, isJudge);
    }
    return ret;
}

static bool isLined(GAME_T *p, uint8_t x, uint8_t y, int8_t vx, int8_t vy, bool isJudge)
{
    uint8_t attrCnt[PIECE_ATTRS];
    memset(attrCnt, 0, PIECE_ATTRS);
    for (uint8_t i = 0; i < BOARD_SIZE; i++) {
        uint8_t piece = p->board[y][x];
        if (piece == BOARD_EMPTY) {
            return false;
        }
        for (uint8_t j = 0; j < PIECE_ATTRS; j++) {
            if (piece & 1 << j) attrCnt[j]++;
        }
        if (vx == 0 && vy == 0) {
            int8_t b = i & 1; 
            x += 1 - b * 2;
            y += b;
        } else {
            x += vx;
            y += vy;
        }
    }
    for (uint8_t i = 0; i < PIECE_ATTRS; i++) {
        if (attrCnt[i] == 0 || attrCnt[i] == BOARD_SIZE) {
            if (isJudge) {
                for (uint8_t i = 0; i < BOARD_SIZE; i++) {
                    if (vx == 0 && vy == 0) {
                        int8_t b = i & 1; 
                        x += 1 - b * 2;
                        y -= 1 - b;
                    } else {
                        x -= vx;
                        y -= vy;
                    }
                    linedPiecesPos |= 1 << (y * BOARD_SIZE + x);
                }
            }
            return true;
        }
    }
    return false;
}

/*---------------------------------------------------------------------------*/
/*                               Menu Handlers                               */
/*---------------------------------------------------------------------------*/

static void onContinue(void)
{
    playSoundClick();
    state = STATE_PLACING;
    isInvalid = true;
    dprintln(F("Menu: continue"));
}

static void onCancel(void)
{
    arduboy.playScore2(soundCancel, SND_PRIO_CANCEL);
    game = lastGame;
    isCancelable = false;
    countWinMoves(&game, false, true);
    state = STATE_PLACING;
    isInvalid = true;
    dprintln(F("Menu: cancel"));
}

static void onRetry(void)
{
    dprintln(F("Menu: retry"));
    initGame();
}

static void onQuit(void)
{
    playSoundClick();
    clearMenuItems();
    addMenuItem(F("QUIT"), onExit);
    addMenuItem(F("CANCEL"), onContinue);
    setMenuCoords(40, 38, 47, 11, true, true);
    setMenuItemPos(1);
    isInvalid = true;
    dprintln(F("Menu: quit"));
}

static void onExit(void)
{
    playSoundClick();
    state = STATE_LEAVE;
    dprintln(F("Back to title"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawBoard(bool isFullUpdate)
{
    static uint8_t lastCursorX, lastCursorY;
    bool isPlayerTurn = (state == STATE_PLACING && !isCpuTurn());
    bool isDrawHint = (state == STATE_PLACING && isShowHint());

    if (isFullUpdate) {
        /*  Board  */
        drawBoardEdge(30);
        drawBoardEdge(96);
        for (uint8_t y = 0; y < BOARD_SIZE - 1; y++) {
            for (uint8_t x = 0; x < BOARD_SIZE - 1; x++) {
                arduboy.drawPixel(x * 16 + 47, y * 16 + 15, WHITE);
            }
        }
        /*  Pieces  */
        uint8_t shake = (counter >> 2);
        for (uint8_t y = 0; y < BOARD_SIZE; y++) {
            for (uint8_t x = 0; x < BOARD_SIZE; x++) {
                bool isShakePiece = linedPiecesPos & 1 << (y * BOARD_SIZE + x);
                shake = (shake + isShakePiece) & 3;
                drawBoardUnit(x, y, isDrawHint, (isShakePiece) ? shake : -1);
            }
        }
    } else {
        if (isPlayerTurn) {
            arduboy.fillRect2(lastCursorX * 16 + 32, lastCursorY * 16, IMG_PIECE_W, IMG_PIECE_H, BLACK);
            drawBoardUnit(lastCursorX, lastCursorY, isDrawHint, -1);
        }
    }
    /*  Cursor  */
    if (isPlayerTurn) {
        drawCursor(cursorX * 16 + 32, cursorY * 16);
        lastCursorX = cursorX;
        lastCursorY = cursorY;
    }
}

static void drawBoardEdge(int16_t x)
{
    uint8_t *p = arduboy.getBuffer() + x;
    for (uint8_t i = 0; i < 8; i++) {
        *p = 0xAA;
        p += WIDTH;
    }
}

static void drawBoardUnit(uint8_t x, uint8_t y, bool isDrawHint, int8_t shake)
{
    uint8_t piece = game.board[y][x];
    int16_t dx = x * 16 + 32, dy = y * 16;
    if (piece != BOARD_EMPTY) {
        if (shake >= 0) {
            if (shake & 1) { // 1 or 3
                dy += 2 - shake;
            } else { // 0 or 2
                dx += 1 - shake;
            }
        }
        drawPiece(dx, dy, piece);
    }
    if (isDrawHint && (hintWinMoves & (1 << (y * BOARD_SIZE + x)))) {
        arduboy.drawBitmap(dx + 5, dy + 5, imgHintWin, IMG_HINT_WIN_W, IMG_HINT_WIN_H, WHITE);
    }
}

static void drawPlayers(bool isFullUpdate)
{
    int a = counter >> 3 & 3;
    for (int i = 0; i < 2; i++) {
        bool isMyTurn = (i == game.turn % 2 && game.turn < TURN_MAX);
        bool isCpu = (1 - i == record.gameMode);
        int x = i * 96 + 10;
        int y = (isMyTurn) ? 21 + abs(a - 1) : 22;
        if (isFullUpdate) {
            if (record.gameMode == GAME_MODE_2PLAYERS) {
                arduboy.printEx(x, 14, (i == 0) ? F("1P") : F("2P"));
            } else {
                arduboy.printEx(x - 3, 14, (isCpu) ? F("CPU") : F("YOU"));
            }
            if (isMyTurn) {
                if (state == STATE_PLACING) drawPiece(x - 2, 40, game.currentPiece);
                arduboy.drawRect2(x - 4, 38, 19, 19, WHITE);
            }
        } else {
            arduboy.fillRect2(x, 21, IMG_PLAYER_W, IMG_PLAYER_H + 2, BLACK);
        }
        arduboy.drawBitmap(x, y, imgPlayer[isCpu], IMG_PLAYER_W, IMG_PLAYER_H, WHITE);
        if (isCpu) drawNumber(x + 3, y + 3, record.cpuLevel);
    }
}

static void drawMovingPiece(void)
{
    int16_t a = animCounter * animCounter;
    int16_t x = (animSx * a + animDx * (ANIM_RESOLUTION - a)) >> 8;
    int16_t y = (animSy * a + animDy * (ANIM_RESOLUTION - a)) >> 8;
    drawPiece(x, y, game.currentPiece);
}

static void drawRestPieces(bool isFullUpdate)
{
    static uint8_t lastCursorPiece;
    bool isDrawHint = isShowHint();
    if (isDrawHint && hintNgPieces && !(counter & 15)) isFullUpdate = true;
    if (game.turn < 7 && lastCursorPiece != cursorPiece) isFullUpdate = true;

    /*  Frame  */
    if (isFullUpdate) {
        arduboy.drawRect2(-1, 38, WIDTH + 2, 19, WHITE);
        arduboy.fillRect2(0, 39, WIDTH, 17, BLACK);
    }
    /*  Pieces  */
    int16_t x = restPiecesLeft;
    for (uint8_t piece = 0; piece < PIECE_MAX; piece++) {
        if (!(game.restPieces & 1 << piece)) continue;
        if (!isFullUpdate && piece == lastCursorPiece) {
            arduboy.fillRect2(x, 40, IMG_PIECE_W, IMG_PIECE_H, BLACK);
        }
        if (isFullUpdate || piece == lastCursorPiece) {
            drawRestPiecesUnit(x, piece, isDrawHint);
        }
        if (piece == cursorPiece) {
            drawCursor(x, 40);
        }
        x += 16;
    }
    lastCursorPiece = cursorPiece;
}

static void drawRestPiecesUnit(int16_t x, uint8_t piece, bool isDrawHint)
{
    drawPiece(x, 40, piece);
    if (isDrawHint && (hintNgPieces & 1 << piece) && (counter & 16)) {
        arduboy.fillRect2(x + 1, 49, 13, 7, BLACK);
        arduboy.printEx(x + 2, 50, F("NG"));
    }
}

static void drawResult(bool isFullUpdate)
{
    if (animCounter > FPS) return;
    if (!isFullUpdate && animCounter == 0) return;
    int h = min(FPS - animCounter, 12);
    arduboy.fillRect2(0, 47 - h / 2, WIDTH, h + 2, BLACK);    
    arduboy.fillRect2(0, 48 - h / 2, WIDTH, h, WHITE);
    int x = WIDTH / 2 - (strlen_PF((uint_farptr_t) resultLabel) * 6 - 1);
    x += animCounter * animCounter / 18;
    if (x < WIDTH) {
        arduboy.setTextColor(BLACK, BLACK);
        arduboy.setTextSize(2);
        arduboy.printEx(x, 43, resultLabel);
        arduboy.setTextColor(WHITE, BLACK);
        arduboy.setTextSize(1);
    }
}

void drawPiece(int16_t x, int16_t y, int16_t piece)
{
    arduboy.drawBitmap(x, y, imgPiece[piece], IMG_PIECE_W, IMG_PIECE_H, WHITE);
}

static void drawCursor(int16_t x, int16_t y)
{
    if (counter & 4) {
        for (uint8_t i = 0; i < 2; i++) {
            arduboy.drawBitmap(x, y, imgCursor[i], IMG_CURSOR_W, IMG_CURSOR_H, 1 - i);
        }
    }
}

/*---------------------------------------------------------------------------*/
/*                            Thinking Algorithm                             */
/*---------------------------------------------------------------------------*/

static bool cpuThinking(void)
{
    GAME_T work = game;
    int8_t depth = min(record.cpuLevel, game.turn / 2 + 1);
    nextCpuInterval = millis() + CPU_INTERVAL_MILLIS;
    isCpuInterrupted = false;
    isLastAPressed = arduboy.pressed(A_BUTTON);
    int eval = -alphabeta(&work, depth, -EVAL_INF, EVAL_INF);
    arduboy.setRGBled(0, 0, 0);
    if (isCpuInterrupted) {
        dprintln(F("CPU was interrupted"));
        return false;
    } else {
        dprint(F("CPU's evaluation="));
        dprintln(eval);
        return true;
    }
}

static int alphabeta(GAME_T *p, int8_t depth, int alpha, int beta)
{
    /*  Can win or is last move?  */
    bool isRoot = (alpha == -EVAL_INF && beta == EVAL_INF);
    int eval = countWinMoves(p, isRoot, false) * EVAL_WIN;
    if (eval > 0 || p->turn >= TURN_MAX - 1 || depth == 0) {
        eval = eval / (TURN_MAX - p->turn) + random(-128, 128);
        if (eval > alpha) {
            alpha = eval;
        }
        return -alpha;
    }

    /*  Search best move  */
    p->turn++;
    for (uint8_t y = 0; y < BOARD_SIZE; y++) {
        for (uint8_t x = 0; x < BOARD_SIZE; x++) {
            if (p->board[y][x] == BOARD_EMPTY) {
                if (millis() >= nextCpuInterval) cpuThinkingInterval();
                if (isCpuInterrupted) {
                    p->turn--;
                    return EVAL_INF;
                }
                p->board[y][x] = p->currentPiece;
                for (uint8_t piece = 0; piece < PIECE_MAX; piece++) {
                    uint16_t pieceBit = 1 << piece;
                    if (p->restPieces & pieceBit) {
                        p->currentPiece = piece;
                        p->restPieces ^= pieceBit;
                        eval = alphabeta(p, depth - 1, -beta, -alpha);
                        p->restPieces ^= pieceBit;
                        if (eval > alpha) {
                            alpha = eval;
                            if (isRoot) {
                                cpuX = x;
                                cpuY = y;
                                cpuPiece = piece;
                            }
                        }
                        if (alpha >= beta) {
                            p->currentPiece = p->board[y][x];
                            p->board[y][x] = BOARD_EMPTY;
                            p->turn--;
                            return -alpha;
                        }
                    }
                }
                p->currentPiece = p->board[y][x];
                p->board[y][x] = BOARD_EMPTY;
            }
        }
    }
    p->turn--;
    return -alpha;
}

static void cpuThinkingInterval(void)
{
    record.playFrames += CPU_INTERVAL_FRAMES;
    counter += CPU_INTERVAL_FRAMES;
    nextCpuInterval += CPU_INTERVAL_MILLIS;
    bool isAPressed = arduboy.pressed(A_BUTTON);
    if (!isLastAPressed && isAPressed) {
        isCpuInterrupted = true;
    } else {
        drawGame();
        arduboy.display();
        if (record.settings & SETTING_BIT_THINK_LED) {
            uint8_t r = counter << 1 & 0x70;
            arduboy.setRGBled((r < 64) ? r + 4 : 132 - r, 0, 0);
        }
    }
    isLastAPressed = isAPressed;
}
