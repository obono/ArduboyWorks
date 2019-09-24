#include "common.h"
#include "data.h"

/*  Defines  */

#define BOARD_SIZE  4
#define BOARD_EMPTY 0xFF
#define PIECE_ATTRS 4
#define TURN_MAX    (BOARD_SIZE * BOARD_SIZE)
#define PIECE_MAX   (1 << PIECE_ATTRS)

#define CPU_INTERVAL_FRAMES 8
#define CPU_INTERVAL_MILLIS ((1000 * CPU_INTERVAL_FRAMES) / FPS)

#define EVAL_INF    32767
#define EVAL_WIN    5400

enum STATE_T {
    STATE_INIT = 0,
    STATE_PLACING,
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

static void newGame(void);
static void finalizeGame(void);
static bool isCpuTurn(void);
static void handlePlacing(void);
static void handleChoosing(void);
static void handleOver(void);
static void onContinue(void);
static void onCancel(void);
static void onRetry(void);
static void onQuit(void);
static void onExit(void);

static void drawBoard(bool isAnimation);
static void drawPiece(int16_t x, int16_t y, int16_t piece);
static void drawCursor(void);
static void drawBoardEdge(int16_t x);
static void drawRestPieces(void);
static void drawPlayerIndication(bool isAnimation);
static void drawResult(void);

static bool cpuThinking(void);
static void cpuThinkingInterval(void);
static int  alphabeta(GAME_T *p, int8_t depth, int alpha, int beta);
static bool isWin(GAME_T *p, uint8_t x, uint8_t y);
static bool isLined(GAME_T *p, uint8_t x, uint8_t y, int8_t vx, int8_t vy);

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static GAME_T   game, lastGame;
static bool     isCancelable, isCpuInterrupted, isLastAPressed;
static uint8_t  animCounter, counter;
static uint8_t  cursorX, cursorY, cursorPiece, cpuX, cpuY, cpuPiece;
static unsigned long nextCpuInterval;
static const byte *resultSound;
static const __FlashStringHelper *resultLabel;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    newGame();
    isRecordDirty = true;
    writeRecord();
    isCancelable = false;
    arduboy.playScore2(soundStart, 0);
    isInvalid = true;
}

MODE_T updateGame(void)
{
    counter++;
    if (state == STATE_PLACING || state == STATE_CHOOSING) {
        record.playFrames++;
        isRecordDirty = true;
    }
    switch (state) {
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

    if (true/*isInvalid*/) {
        arduboy.clear();
        drawBoard(false);
        drawPlayerIndication(false);
    } else {
        // TODO
    }

    switch (state) {
    case STATE_PLACING:
        if (!isCpuTurn()) drawCursor();
        break;
    case STATE_CHOOSING:
        drawRestPieces();
        break;
    case STATE_OVER:
        drawResult();
        break;
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void newGame(void)
{
    record.playCount++;
    memset(&game.board, BOARD_EMPTY, BOARD_SIZE * BOARD_SIZE);
    cursorX = 0;
    cursorY = 0;
    cursorPiece = random(16);
    game.turn = 0;
    game.currentPiece = cursorPiece;
    game.restPieces = ~(1 << cursorPiece);
    state = STATE_PLACING;
    dprintln(F("New game"));
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
    animCounter = 0;
    state = STATE_OVER;
    dprint(F("Game set: "));
    dprintln(resultLabel);
}

static bool isCpuTurn(void)
{
    return (record.gameMode != GAME_MODE_2PLAYERS && game.turn % 2 != record.gameMode);
}

static void handlePlacing(void)
{
    bool isPlaced = false;
    if (isCpuTurn()) {
        /*  CPU thinking  */
        if (!arduboy.buttonDown(A_BUTTON)) isPlaced = cpuThinking();
        if (isPlaced) {
            cursorX = cpuX;
            cursorY = cpuY;
        }
    } else {
        /*  Move cursor  */
        handleDPad();
        if (padX != 0 || padY != 0) {
            cursorX = (cursorX + padX + BOARD_SIZE) % BOARD_SIZE;
            cursorY = (cursorY + padY + BOARD_SIZE) % BOARD_SIZE;
            playSoundTick();
        }
        /*  Place a piece  */
        if (arduboy.buttonDown(B_BUTTON) && game.board[cursorY][cursorX] == BOARD_EMPTY) {
            isPlaced = true;
        }
    }

    if (isPlaced) {
        game.board[cursorY][cursorX] = game.currentPiece;
        arduboy.playScore2(soundPlace, SND_PRIO_PLACE);
        if (isWin(&game, cursorX, cursorY)) {
            finalizeGame();
        } else if (game.restPieces == 0) {
            game.turn++;
            finalizeGame();
        } else {
            while (!(game.restPieces & 1 << cursorPiece)) {
                cursorPiece = (cursorPiece + 1) % PIECE_MAX;
            }
            state = STATE_CHOOSING;
        }
    } else if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        clearMenuItems();
        addMenuItem(F("CONTINUE"), onContinue);
        if (isCancelable) addMenuItem(F("CANCEL LAST MOVE"), onCancel);
        addMenuItem(F("QUIT GAME"), onQuit);
        int w = (isCancelable) ? 107 : 83; 
        int h = (isCancelable) ? 17 : 11;
        setMenuCoords(63 - w / 2, 32 - h / 2, w, h, true, true);
        setMenuItemPos(0);
        isCpuInterrupted = false;
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
    bool isChosen = false;
    if (isCpuTurn()) {
        cursorPiece = cpuPiece;
        isChosen = true;
    } else {
        handleDPad();
        if (padX != 0) {
            do {
                cursorPiece = (cursorPiece + padX + PIECE_MAX) % PIECE_MAX;
            } while (!(game.restPieces & 1 << cursorPiece));
            playSoundTick();
        }
        if (arduboy.buttonDown(B_BUTTON)) {
            /*  Choose  */
            isChosen = true;
        } else if (arduboy.buttonDown(A_BUTTON)) {
            /*  Cancel  */
            game.board[cursorY][cursorX] = BOARD_EMPTY;
            state = STATE_PLACING;
        }
    }

    if (isChosen) {
        game.turn++;
        game.currentPiece = cursorPiece;
        game.restPieces &= ~(1 << cursorPiece);
        state = STATE_PLACING;
        playSoundClick();
    }
}

static void handleOver(void)
{
    if (animCounter <= FPS * 2) {
        if (animCounter == 0) writeRecord();
        animCounter++;
        if (animCounter == FPS) arduboy.playScore2(resultSound, SND_PRIO_RESULT);
        if (animCounter == FPS * 2 && resultSound != soundLose) arduboy.stopScore2();
    } else {
        if (arduboy.buttonDown(B_BUTTON)) isInvalid = true;
        if (arduboy.buttonUp(B_BUTTON)) animCounter = FPS * 2;
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
    state = STATE_PLACING;
    isInvalid = true;
}

static void onCancel(void)
{
    game = lastGame;
    isCancelable = false;
    arduboy.playScore2(soundCancel, SND_PRIO_CANCEL);
    state = STATE_PLACING;
    isInvalid = true;
}

static void onRetry(void)
{
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
    writeRecord();
    state = STATE_LEAVE;
    dprintln(F("Back to title"));
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawBoard(bool isAnimation)
{
    /*  Board  */
    drawBoardEdge(30);
    drawBoardEdge(96);
    for (uint8_t y = 0; y < BOARD_SIZE - 1; y++) {
        for (uint8_t x = 0; x < BOARD_SIZE - 1; x++) {
            arduboy.drawPixel(x * 16 + 47, y * 16 + 15, WHITE);
        }
    }

    /*  Pieces  */
    for (uint8_t y = 0; y < BOARD_SIZE; y++) {
        for (uint8_t x = 0; x < BOARD_SIZE; x++) {
            uint8_t piece = game.board[y][x];
            if (piece != BOARD_EMPTY) {
                drawPiece(x * 16 + 32, y * 16, piece);
            }
        }
    }
}

static void drawPiece(int16_t x, int16_t y, int16_t piece)
{
    arduboy.drawBitmap(x, y, imgPiece[piece], IMG_PIECE_W, IMG_PIECE_H, WHITE);
}

static void drawCursor(void)
{
    int8_t x = cursorX * 16 + 32;
    int8_t y = cursorY * 16;
    arduboy.drawRect2(x, y, IMG_PIECE_W, IMG_PIECE_H, counter % 2);
}

static void drawBoardEdge(int16_t x)
{
    uint8_t *p = arduboy.getBuffer() + x;
    for (uint8_t i = 0; i < 8; i++) {
        *p = 0xAA;
        p += WIDTH;
    }
}

static void drawRestPieces(void)
{
    arduboy.drawRect2(-1, 38, WIDTH + 2, 19, WHITE);
    arduboy.fillRect2(0, 39, WIDTH, 17, BLACK);

    uint8_t piecesCount = TURN_MAX - 1 - game.turn;
    uint8_t piecePos = 0;
    for (uint8_t piece = 0; piece < cursorPiece; piece++) {
        if (game.restPieces & 1 << piece) piecePos++;
    }
    int16_t x = 64 - piecesCount * 8;
    if (piecesCount > 8) {
        int16_t gap = (piecesCount - 8) * 16;
        x += gap / 2 - gap * piecePos / (piecesCount - 1);
    }
    for (uint8_t i = 0, piece = 0; i < piecesCount; i++, piece++) {
        while (!(game.restPieces & 1 << piece)) {
            piece++;
        }
        drawPiece(x, 40, piece);
        if (piece == cursorPiece) {
            arduboy.drawRect2(x, 40, IMG_PIECE_W, IMG_PIECE_H, counter % 2); // TODO
        }
        x += 16;
    }
}

static void drawPlayerIndication(bool isAnimation)
{
    if (isAnimation) {
        arduboy.fillRect2(0, 24, IMG_PLAYER_W, IMG_PLAYER_H + 8, BLACK);
        arduboy.fillRect2(116, 24, IMG_PLAYER_W, IMG_PLAYER_H + 8, BLACK);
    } else {
        // TODO
    }

    int a = counter >> 3 & 3;
    for (int i = 0; i < 2; i++) {
        bool isCpu = isCpuTurn();
        int x = i * 96 + 10;
        int y = (i == game.turn % 2) ? 21 + abs(a - 1) : 22;
        if (record.gameMode == GAME_MODE_2PLAYERS) {
            arduboy.printEx(x, 14, (i == 0) ? F("1P") : F("2P"));
        } else {
            arduboy.printEx(x - 3, 14, isCpu ? F("CPU") : F("YOU"));
        }
        arduboy.drawBitmap(x, y, imgPlayer[isCpu], IMG_PLAYER_W, IMG_PLAYER_H, WHITE);
        if (isCpu) drawNumber(x + 3, y + 3, record.cpuLevel);
    }

    if (state == STATE_PLACING) {
        uint16_t x = game.turn % 2 * 96 + 8;
        drawPiece(x, 40, game.currentPiece);
        arduboy.drawRect2(x - 2, 38, 19, 19, WHITE);
    }
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
    bool isRoot = (alpha == -EVAL_INF && beta == EVAL_INF);
    int eval = 0;

    /*  Can win or is last move?  */
    for (uint8_t y = 0; y < BOARD_SIZE; y++) {
        for (uint8_t x = 0; x < BOARD_SIZE; x++) {
            if (p->board[y][x] == BOARD_EMPTY) {
                if (isRoot && p->turn >= TURN_MAX - 1) {
                    cpuX = x;
                    cpuY = y;
                }
                p->board[y][x] = p->currentPiece;
                if (isWin(p, x, y)) {
                    eval += EVAL_WIN;
                    if (isRoot) {
                        cpuX = x;
                        cpuY = y;
                    }
                }
                p->board[y][x] = BOARD_EMPTY;
            }
        }
    }
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

static bool isWin(GAME_T *p, uint8_t x, uint8_t y)
{
    bool ret = isLined(p, x, 0, 0, 1) || isLined(p, 0, y, 1, 0);
    if (!ret && x == y) ret = isLined(p, 0, 0, 1, 1);
    if (!ret && x == BOARD_SIZE - y - 1) ret = isLined(p, BOARD_SIZE - 1, 0, -1, 1);
    return ret;
}

static bool isLined(GAME_T *p, uint8_t x, uint8_t y, int8_t vx, int8_t vy)
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
        x += vx;
        y += vy;
    }
    for (uint8_t i = 0; i < PIECE_ATTRS; i++) {
        if (attrCnt[i] == 0 || attrCnt[i] == BOARD_SIZE) {
            return true;
        }
    }
    return false;
}
