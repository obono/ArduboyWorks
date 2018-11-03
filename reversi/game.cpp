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

/*  Typedefs  */

typedef struct {
    uint8_t white[BOARD_H];
    uint8_t black[BOARD_H];
    uint8_t flag[BOARD_H];
    uint8_t numStones, numBlack, numWhite, numPlaceable;
    bool    isWhiteTurn, isLastPassed;
} BOARD_T;

typedef struct {
    uint8_t x:3;
    uint8_t y:3;
} POS_T;

/*  Local Functions  */

static void resetAnimationParams(void);
static void evaluateBoard(BOARD_T *p);
static bool isPlaceable(BOARD_T *p, int x, int y, bool isActual);
static bool isReversible(BOARD_T *p, int x, int y, int vx, int vy, bool isActual);
static void handlePlaying(void);
static void handleAnimation(void);
static void handleOver(void);

static void drawBoard(bool isAnimation);
static void drawStone(int x, int y, int anim, bool isBlack);
static void drawCursor(void);
static void drawStrings(void);
static void drawResult(void);

#define placeStone(b, x, y)    isPlaceable(b, x, y, true)

/*  Local Variables  */

static STATE_T  state = STATE_INIT;
static BOARD_T  board;
static int8_t   animTable[BOARD_H][BOARD_W];
static uint8_t  animStones, animCounter;
static uint8_t  ledRGB[3];

static POS_T    cursorPos, lastPos;

static uint32_t gameFrames;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    record.playCount++;
    isRecordDirty = true;
    writeRecord();

    memset(&board, 0, sizeof(board));
    evaluateBoard(&board);
    gameFrames = 0;
    resetAnimationParams();
    arduboy.playScore2(soundStart, 0);
    state = STATE_PLAYING;
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
    if (isInvalid) {
        arduboy.clear();
        drawBoard(false);
        isInvalid = false;
    }
    if (state == STATE_PLAYING) {
        arduboy.setRGBled(ledRGB[0], ledRGB[1], ledRGB[2]);
        if (board.numStones >= 4) {
            drawCursor();
            drawStrings();
        }
    } else {
        arduboy.setRGBled(0, 0, 0);
        if (state == STATE_ANIMATION) drawBoard(true);
        if (state == STATE_OVER) drawResult();
        if (state == STATE_MENU) drawMenuItems(false);
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

static void resetAnimationParams(void)
{
    memset(animTable, -1, sizeof(animTable));
    animStones = 0;
    animCounter = 0;
}

static void evaluateBoard(BOARD_T *p)
{
    p->numBlack = 0;
    p->numWhite = 0;
    p->numPlaceable = 0;
    for (int y = 0; y < BOARD_H; y++) {
        uint8_t black = p->black[y];
        uint8_t white = p->white[y];
        uint8_t flag = 0;
        for (int x = 0; x < BOARD_W; x++) {
            uint8_t b = 1 << x;
            if (black & b) p->numBlack++;
            if (white & b) p->numWhite++;
            if (~(black | white) & b) {
                if (isPlaceable(p, x, y, false)) {
                    p->numPlaceable++;
                    flag |= b;
                }
            }
        }
        p->flag[y] = flag;
    }
    p->numStones = p->numBlack + p->numWhite;
}

static bool isPlaceable(BOARD_T *p, int x, int y, bool isActual)
{
    bool ret = false;
    if (isActual) {
        if (p->isWhiteTurn) {
            p->white[y] |= 1 << x;
        } else {
            p->black[y] |= 1 << x;
        }
        animTable[y][x] = 0;
    }
    for (int vy = -1; vy <= 1; vy++) {
        for (int vx = -1; vx <= 1; vx++) {
            if (vx == 0 && vy == 0) continue;
            if (isReversible(p, x, y, vx, vy, isActual)) {
                if (!isActual) return true;
                ret = true;
            }
        }
    }
    return ret;
}

static bool isReversible(BOARD_T *p, int x, int y, int vx, int vy, bool isActual)
{
    for (int s = 0; ; s++) {
        x += vx;
        y += vy;
        if (x < 0 || y < 0 || x >= BOARD_W || y >= BOARD_H) return false;
        bool isBlack = p->black[y] & 1 << x;
        bool isWhite = p->white[y] & 1 << x;
        if (!isBlack && !isWhite) return false;
        if (isWhite == p->isWhiteTurn) {
            if (s == 0) return false;
            if (isActual) {
                animStones += s;
                int8_t anim = animStones * 4 + 12;
                while (s-- > 0) {
                    x -= vx;
                    y -= vy;
                    p->black[y] ^= 1 << x;
                    p->white[y] ^= 1 << x;
                    animTable[y][x] = anim;
                    anim -= 4;
                }
            }
            return true;
        }
    }
}

static void handlePlaying(void)
{
    int numStones = board.numStones;
    if (numStones < 4) {
        int x = (numStones == 0 || numStones == 3) ? 3 : 4;
        int y = (numStones == 0 || numStones == 1) ? 3 : 4;
        placeStone(&board, x, y);
        state = STATE_ANIMATION;
    } else {
        handleDPad();
        if (padX != 0 || padY != 0) {
            cursorPos.x += padX;
            cursorPos.y += padY;
            isInvalid = true;
        }
        if (arduboy.buttonDown(B_BUTTON)) {
            if (board.numPlaceable > 0) {
                if (board.flag[cursorPos.y] & 1 << cursorPos.x) {
                    placeStone(&board, cursorPos.x, cursorPos.y);
                    state = STATE_ANIMATION;
                }
            } else {
                if (board.isLastPassed) {
                    writeRecord();
                    arduboy.playScore2(soundOver, 1);
                    state = STATE_OVER;
                } else {
                    board.isWhiteTurn = !board.isWhiteTurn;
                    board.isLastPassed = true;
                    evaluateBoard(&board);
                }
                isInvalid = true;
            }
        }
        if (arduboy.buttonDown(A_BUTTON)) {
            writeRecord();
            state = STATE_LEAVE;
        }
    }
}

static void handleAnimation(void)
{
    animCounter++;
    if (animCounter >= animStones * 4 + 12) {
        board.isWhiteTurn = !board.isWhiteTurn;
        board.isLastPassed = false;
        evaluateBoard(&board);
        resetAnimationParams();
        if (board.numStones == BOARD_W * BOARD_H) {
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

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawBoard(bool isAnimation)
{
    for (int y = 0; y < BOARD_H; y++) {
        uint8_t black = board.black[y];
        uint8_t white = board.white[y];
        uint8_t flag = board.flag[y];
        uint8_t placeable = flag & ~(black | white);
        int dy = y * 8;
        int anim = -1;
        for (int x = 0; x < BOARD_W; x++) {
            int dx = x * 12 + 16;
            uint8_t b = 1 << x;
            if (isAnimation) {
                anim = animTable[y][x] - animCounter;
                int belowAnim = (y < BOARD_H - 1) ? animTable[y + 1][x] - animCounter : -1;
                if ((anim < 0 || anim >= 16) && (belowAnim < 0 || belowAnim >= 16)) continue; // skip drawing
                arduboy.fillRect(dx, dy, 11, 8, BLACK);
            } else {
                if (x < BOARD_W - 1 && y < BOARD_H - 1) {
                    arduboy.drawPixel(dx + 11, dy + 8, WHITE);
                }
            }
            if (anim < 0) { // usual drawing
                if (black & b) drawStone(dx, dy, 0, true);
                if (white & b) drawStone(dx, dy, 0, false);
                /*if (state == STATE_PLAYING && (placeable & b)) {
                    arduboy.drawFastVLine2(x * 12 + 21, y * 8 + 3, 3, WHITE);
                    arduboy.drawFastHLine2(x * 12 + 20, y * 8 + 4, 3, WHITE); // placeable mark
                }*/
            } else if (anim < 16) {
                drawStone(dx, dy, anim, !board.isWhiteTurn);
            } else {
                drawStone(dx, dy, 0, board.isWhiteTurn);
            }
        }
    }
    if (!isAnimation) {
        arduboy.drawFastVLine2(14, 0, HEIGHT, WHITE);
        arduboy.drawFastVLine2(112, 0, HEIGHT, WHITE);
    }
}

static void drawStone(int x, int y, int anim, bool isBlack) {
    arduboy.drawBitmap(x, y - 8, imgStoneBase[anim], 12, 16, WHITE);
    if (anim > 4) isBlack = !isBlack;
    if (isBlack) {
        arduboy.drawBitmap(x, y - 8, imgStoneFace[anim], 12, 16, BLACK);
    }
}

static void drawCursor(void) {
    bool isBlink = (gameFrames & 4);
    if (board.numPlaceable > 0) {
        arduboy.drawRect(cursorPos.x * 12 + 16, cursorPos.y * 8 + 1, 11, 7, isBlink ? WHITE : BLACK);
    } else {
        arduboy.fillRect2(50, 27, 27, 9, WHITE);
        arduboy.fillRect2(51, 28, 25, 7, BLACK);
        if (isBlink) arduboy.printEx(52, 29, F("PASS"));
    }
}

static void drawStrings(void)
{
    drawNumber(0, 0, board.numBlack);
    drawNumber(116, 0, board.numWhite);
    arduboy.drawFastHLine2(board.isWhiteTurn ? 116 : 0, 6, 11, WHITE);
}

static void drawResult(void)
{
    arduboy.fillRect2(35, 27, 57, 9, WHITE);
    arduboy.fillRect2(36, 28, 55, 7, BLACK);
    arduboy.printEx(37, 29, F("GAME OVER"));
}

