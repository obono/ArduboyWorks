#include "common.h"

/*  Defines  */

#define PIECES  10
#define BOARD_W 17
#define BOARD_H 9

#define EMPTY   -1
#define COLUMN  -2

#define KEY_REPEAT_DELAY    15
#define KEY_REPEAT_INTERVAL 5

enum STATE_T {
    STATE_INIT = 0,
    STATE_FREE,
    STATE_PICKED,
    STATE_CLEAR,
};

enum ALIGN_T {
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT,
};

/*  Typedefs  */

typedef struct {
    uint16_t    x : 5;
    uint16_t    y : 4;
    uint16_t    rot : 3;
    uint16_t    dummy : 4;
} PIECE_T;

/*  Local Functions  */

static bool putPieces(void);
static bool putPiece(int8_t idx);
static bool putPiecePart(int8_t idx, int8_t x, int8_t y, int8_t c);
static bool handleButtons(void);
static bool handleButtonsFree(int8_t vx, int8_t vy);
static void handleButtonsPicked(int8_t vx, int8_t vy);

static void drawBoard(void);
static void drawCursor(void);
static void drawPieces(void);
static void drawPiece(int8_t idx);
static bool drawPiecePart(int8_t idx, int8_t x, int8_t y, int8_t c);
static bool execPiece(int8_t idx, bool (*f)(int8_t, int8_t, int8_t, int8_t));

/*  Local Variables  */

PROGMEM static const int8_t piecePtn[PIECES][16] = {
    { 0x5, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,  0x2, 0x9, 0x9, 0x9,  0x9, 0xA, 0x0, 0x0 }, // white
    { 0x0, 0x0, 0x5, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x8, 0xB, 0x9,  0x9, 0xA, 0x0, 0x0 }, // green
    { 0x0, 0x5, 0x0, 0x0,  0x0, 0x5, 0x0, 0x0,  0x0, 0x2, 0x9, 0x9,  0x9, 0x1, 0x0, 0x0 }, // purple
    { 0x4, 0x9, 0xE, 0xA,  0x7, 0x0, 0x7, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0 }, // orange
    { 0x4, 0x9, 0x9, 0xA,  0x6, 0x0, 0x0, 0x0,  0x2, 0xA, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0 }, // red
    { 0x0, 0x0, 0x4, 0xA,  0x5, 0x0, 0x6, 0x0,  0x2, 0x9, 0x1, 0x0,  0x0, 0x0, 0x0, 0x0 }, // pink
    { 0x0, 0x8, 0xE, 0xA,  0x0, 0x0, 0x6, 0x0,  0x0, 0x8, 0x1, 0x0,  0x0, 0x0, 0x0, 0x0 }, // gray
    { 0x0, 0x0, 0x4, 0xA,  0x0, 0x0, 0x6, 0x0,  0x4, 0x9, 0x1, 0x0,  0x7, 0x0, 0x0, 0x0 }, // blue
    { 0x0, 0x5, 0x0, 0x0,  0x8, 0xF, 0xA, 0x0,  0x0, 0x7, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0 }, // brown
    { 0x0, 0x5, 0x0, 0x0,  0x0, 0xD, 0xA, 0x0,  0x0, 0x6, 0x0, 0x0,  0x8, 0x1, 0x0, 0x0 }, // yellow
};

PROGMEM static const int8_t pieceRot[8][16] = {
    { 0x0, 0x1, 0x2, 0x3,  0x4, 0x5, 0x6, 0x7,  0x8, 0x9, 0xA, 0xB,  0xC, 0xD, 0xE, 0xF },
    { 0x0, 0x2, 0x4, 0x1,  0x3, 0xA, 0x9, 0x8,  0x5, 0x6, 0x7, 0xD,  0xB, 0xE, 0xC, 0xF },
    { 0x0, 0x4, 0x3, 0x2,  0x1, 0x7, 0x6, 0x5,  0xA, 0x9, 0x8, 0xE,  0xD, 0xC, 0xB, 0xF },
    { 0x0, 0x3, 0x1, 0x4,  0x2, 0x8, 0x9, 0xA,  0x7, 0x6, 0x5, 0xC,  0xE, 0xB, 0xD, 0xF },
    { 0x0, 0x3, 0x4, 0x1,  0x2, 0x7, 0x6, 0x5,  0x8, 0x9, 0xA, 0xE,  0xC, 0xD, 0xB, 0xF },
    { 0x0, 0x1, 0x3, 0x2,  0x4, 0x8, 0x9, 0xA,  0x5, 0x6, 0x7, 0xC,  0xB, 0xE, 0xD, 0xF },
    { 0x0, 0x2, 0x1, 0x4,  0x3, 0x5, 0x6, 0x7,  0xA, 0x9, 0x8, 0xB,  0xD, 0xC, 0xE, 0xF },
    { 0x0, 0x4, 0x2, 0x3,  0x1, 0xA, 0x9, 0x8,  0x7, 0x6, 0x5, 0xD,  0xE, 0xB, 0xC, 0xF },
};

PROGMEM static const PIECE_T pieceAryDefault[PIECES] = {
    { 0,  3, 1, 0 }, { 3,  2, 3, 0 }, { 1,  3, 1, 0 }, { 14, 1, 0, 0 }, { 14, 7, 4, 0 },
    { 14, 6, 3, 0 }, { 15, 1, 1, 0 }, { 14, 4, 3, 0 }, { 1,  7, 0, 0 }, { 3,  6, 6, 0 },
};

PROGMEM static const uint8_t imgPieceBody[16][7] = { // 7x7 x16
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },{ 0x1F, 0x1F, 0x0F, 0x07, 0x03, 0x00, 0x00 },
    { 0x00, 0x00, 0x03, 0x07, 0x0F, 0x1F, 0x1F },{ 0x7C, 0x7C, 0x78, 0x70, 0x60, 0x00, 0x00 },
    { 0x00, 0x00, 0x60, 0x70, 0x78, 0x7C, 0x7C },{ 0x00, 0x7C, 0x7E, 0x7E, 0x7E, 0x7C, 0x00 },
    { 0x00, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x00 },{ 0x00, 0x1F, 0x3F, 0x3F, 0x3F, 0x1F, 0x00 },
    { 0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E },{ 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E },
    { 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x1C, 0x00 },{ 0x1F, 0x1F, 0x0F, 0x07, 0x0F, 0x1F, 0x1F },
    { 0x7F, 0x7F, 0x7F, 0x77, 0x63, 0x00, 0x00 },{ 0x00, 0x00, 0x63, 0x77, 0x7F, 0x7F, 0x7F },
    { 0x7C, 0x7C, 0x78, 0x70, 0x78, 0x7C, 0x7C },{ 0x7F, 0x7F, 0x7F, 0x77, 0x7F, 0x7F, 0x7F },
};

PROGMEM static const uint8_t imgPieceFrame[16][7] = { // 7x7 x16
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },{ 0x20, 0x20, 0x10, 0x08, 0x04, 0x03, 0x00 },
    { 0x00, 0x03, 0x04, 0x08, 0x10, 0x20, 0x20 },{ 0x02, 0x02, 0x04, 0x08, 0x10, 0x60, 0x00 },
    { 0x00, 0x60, 0x10, 0x08, 0x04, 0x02, 0x02 },{ 0x7C, 0x02, 0x01, 0x01, 0x01, 0x02, 0x7C },
    { 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F },{ 0x1F, 0x20, 0x40, 0x40, 0x40, 0x20, 0x1F },
    { 0x1C, 0x22, 0x41, 0x41, 0x41, 0x41, 0x41 },{ 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 },
    { 0x41, 0x41, 0x41, 0x41, 0x41, 0x22, 0x1C },{ 0x20, 0x20, 0x10, 0x08, 0x10, 0x20, 0x20 },
    { 0x00, 0x00, 0x00, 0x08, 0x14, 0x63, 0x00 },{ 0x00, 0x63, 0x14, 0x08, 0x00, 0x00, 0x00 },
    { 0x02, 0x02, 0x04, 0x08, 0x04, 0x02, 0x02 },{ 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00 },
};

static STATE_T  state;
static bool     toDrawAll;
static int8_t   keyRepeatCount;
static int8_t   cursorX, cursorY;
static int8_t   focusPieceIdx;
static PIECE_T  pieceAry[PIECES];
static int8_t   pieceOrder[PIECES];
static int8_t   board[BOARD_H][BOARD_W];

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initGame(void)
{
    if (state == STATE_INIT) {
        if (!readRecordPieces((uint16_t *) pieceAry)) {
            resetPieces();
        }
        for (int i = 0; i < PIECES; i++) {
            pieceOrder[i] = i;
        }
        cursorX = 8;
        cursorY = 4;
        focusPieceIdx = EMPTY;
    }

    putPieces();
    state = STATE_FREE;
    keyRepeatCount = 0;
}

bool updateGame(void)
{
    bool ret = handleButtons();
    if (state != STATE_CLEAR) {
        playFrames++;
    }
    return ret;
}

void drawGame(void)
{
    if (toDrawAll) {
        arduboy.clear();
        drawBoard();
        drawPieces();
        toDrawAll = false;
    } else {
        if (focusPieceIdx >= 0) {
            drawPiece(focusPieceIdx);
        }
    }
    if (state == STATE_FREE) {
        drawCursor();
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

void resetPieces(void)
{
    memcpy_P(pieceAry, pieceAryDefault, sizeof(PIECE_T) * PIECES);
}

static bool putPieces(void)
{
    for (int y = 0; y < BOARD_H; y++) {
        for (int x = 0; x < BOARD_W; x++) {
            board[y][x] = (x < 4 || x > 12 || (x & y & 1)) ? COLUMN : EMPTY;
        }
    }
    bool ret = false;
    for (int i = 0; i < PIECES; i++) {
        ret = putPiece(pieceOrder[i]) || ret;
    }
    toDrawAll = true;
    return ret;
}

static bool putPiece(int8_t idx)
{
    return execPiece(idx, &putPiecePart);
}

static bool execPiece(int8_t idx, bool (*f)(int8_t, int8_t, int8_t, int8_t))
{
    bool    ret = false;
    PIECE_T *p = &pieceAry[idx];

    int8_t xStart, xEnd, yStart, yEnd;
    if (idx < 3) {
        xStart = -3; xEnd = 4;
        yStart = -1; yEnd = 0;
    } else {
        xStart = -1; xEnd = 2;
        yStart = -1; yEnd = 2;
    }
    int8_t vx = abs(p->rot % 4 - 2) - 1;
    int8_t vy = abs(p->rot % 4 - 1) - 1;
    int8_t vh = (p->rot < 4) ? 1 : -1;
    int8_t const *pPtn = piecePtn[idx];
    int8_t const *pRot = pieceRot[p->rot];
    for (int8_t dy = yStart; dy <= yEnd; dy++) {
        for (int8_t dx = xStart; dx <= xEnd; dx++) {
            int8_t x = p->x + dx * vx + dy * vy * vh;
            int8_t y = p->y - dx * vy + dy * vx * vh;
            ret = f(idx, x, y, pgm_read_byte(pRot + pgm_read_byte(pPtn++))) || ret;
        }
    }
    return ret;

}

static bool putPiecePart(int8_t idx, int8_t x, int8_t y, int8_t c)
{
    bool ret = false;
    if (c > 0) {
        if (x < 0 || y < 0 || x >= BOARD_W || y >= BOARD_H) {
            ret = true;
        } else {
            ret = (board[y][x] != EMPTY);
            if (c >= 5) {
                board[y][x] = idx;
            }
        }
    }
    return ret;
}

static void raisePiece(int8_t idx) {
    int i = 9;
    while (pieceOrder[i] != idx) {
        i--;
    }
    if (i != 9) {
        while (i < 9) {
            pieceOrder[i] = pieceOrder[i + 1];
            i++;
        }
        pieceOrder[9] = idx;
        putPiece(idx);
    }
}

static bool handleButtons(void)
{
    int vx = 0, vy = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON | RIGHT_BUTTON | UP_BUTTON | DOWN_BUTTON)) {
        if (++keyRepeatCount >= (KEY_REPEAT_DELAY + KEY_REPEAT_INTERVAL)) {
            keyRepeatCount = KEY_REPEAT_DELAY;
        }
        if (keyRepeatCount == 1 || keyRepeatCount == KEY_REPEAT_DELAY) {
            if (arduboy.buttonPressed(LEFT_BUTTON))  vx--;
            if (arduboy.buttonPressed(RIGHT_BUTTON)) vx++;
            if (arduboy.buttonPressed(UP_BUTTON))    vy--;
            if (arduboy.buttonPressed(DOWN_BUTTON))  vy++;
        }
    } else {
        keyRepeatCount = 0;
    }

    bool ret = false;
    if (state == STATE_FREE) {
        ret = handleButtonsFree(vx, vy);
    } else if (state == STATE_PICKED) {
        handleButtonsPicked(vx, vy);
    }
    return ret;
}

static bool handleButtonsFree(int8_t vx, int8_t vy)
{
    bool ret = false;
    if (cursorX + vx < 0 || cursorX + vx >= BOARD_W) vx = 0;
    if (cursorY + vy < 0 || cursorY + vy >= BOARD_H) vy = 0;
    if (vx != 0 || vy != 0) {
        cursorX += vx;
        cursorY += vy;
        focusPieceIdx = board[cursorY][cursorX];
        if (focusPieceIdx >= 0) {
            raisePiece(focusPieceIdx);
        }
        toDrawAll = true;
    }
    if (arduboy.buttonDown(B_BUTTON) && focusPieceIdx >= 0) {
        playSoundClick();
        state = STATE_PICKED;
        toDrawAll = true;
    } else if (arduboy.buttonDown(A_BUTTON)) {
        saveRecord((uint16_t *)pieceAry);
        ret = true;
    }
    return ret;
}

static void handleButtonsPicked(int8_t vx, int8_t vy)
{
    PIECE_T *p = &pieceAry[focusPieceIdx];
    if (arduboy.buttonPressed(A_BUTTON)) {
        int vr = 0;
        if (arduboy.buttonDown(LEFT_BUTTON))  vr--;
        if (arduboy.buttonDown(RIGHT_BUTTON)) vr++;
        if (vr != 0) {
            p->rot = (p->rot + vr) & 3 | p->rot & 4; // turn
            toDrawAll = true;
        } else if (arduboy.buttonDown(UP_BUTTON | DOWN_BUTTON)) {
            p->rot = (4 - p->rot) & 3 | ~p->rot & 4; // flip
            toDrawAll = true;
        }
    } else {
        int g = (focusPieceIdx < 3) ? 0 : 1;
        if (p->x + vx < g || p->x + vx >= BOARD_W - g) vx = 0;
        if (p->y + vy < g || p->y + vy >= BOARD_H - g) vy = 0;
        if (vx != 0 || vy != 0) {
            p->x += vx;
            p->y += vy;
            toDrawAll = true;
        }
        if (arduboy.buttonDown(B_BUTTON)) {
            playSoundClick();
            putPieces(); // ToDo: Judge
            state = STATE_FREE;
            cursorX = p->x;
            cursorY = p->y;
            toDrawAll = true;
        }
    }
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawBoard(void)
{
    arduboy.drawFastVLine2(31, 0, 64, WHITE);
    arduboy.drawFastVLine2(95, 0, 64, WHITE);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            arduboy.drawRect2(x * 14 + 39, y * 14 + 7, 7, 7, WHITE);
        }
    }
}

static void drawCursor(void)
{
    arduboy.drawRect2(cursorX * 7 + 4, cursorY * 7, 7, 7, WHITE);
    arduboy.fillRect2(cursorX * 7 + 5, cursorY * 7 + 1, 5, 5, playFrames & 1);
}

static void drawPieces(void)
{
    for (int i = 0; i < PIECES; i++) {
        execPiece(pieceOrder[i], &drawPiecePart);
    }
}

static void drawPiece(int8_t idx)
{
    execPiece(idx, &drawPiecePart);
}

static bool drawPiecePart(int8_t idx, int8_t x, int8_t y, int8_t c)
{
    uint8_t bodyCol = WHITE, frameCol = BLACK;
    if (idx == focusPieceIdx) {
        if (state == STATE_PICKED) {
            bodyCol = playFrames & 1;
        }
        frameCol = ((playFrames & 7) < 4) ? WHITE : BLACK;
    }
    arduboy.drawBitmap(x * 7 + 4, y * 7, imgPieceBody[c], 7, 7, bodyCol);
    arduboy.drawBitmap(x * 7 + 4, y * 7, imgPieceFrame[c], 7, 7, frameCol);
    return false;
}
