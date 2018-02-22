#include "common.h"

/*  Defines  */

#define PIECES  10
#define BOARD_W 17
#define BOARD_H 9

#define EMPTY   -1
#define COLUMN  -2

#define PAD_REPEAT_DELAY    15
#define PAD_REPEAT_INTERVAL 5

#define HELP_W  32
#define HELP_H  11
#define HELP_TOP_LIMIT  (HEIGHT - HELP_H)
#define HELP_RIGHT_POS  (WIDTH - HELP_W)

enum STATE_T {
    STATE_INIT = 0,
    STATE_FREE,
    STATE_PICKED,
    STATE_CLEAR,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    uint16_t    x : 6;
    uint16_t    y : 6;
    uint16_t    rot : 4;
} PIECE_T;

typedef struct {
    uint8_t xy : 5;
    uint8_t rot : 3;
} CODE_T;

/*  Local Functions  */

static bool putPieces(void);
static bool putPiece(int8_t idx);
static bool execPiece(int8_t idx, bool(*f)(int8_t, int8_t, int8_t, int8_t));
static bool putPiecePart(int8_t idx, int8_t x, int8_t y, int8_t c);
static void focusPiece(int8_t x, int8_t y);
static void handleDPad(void);
static void moveCursor(int8_t vx, int8_t vy);
static void movePiece(int8_t vx, int8_t vy);
static void adjustHelpPosition(void);

static void drawBoard(void);
static void drawDottedVLine(int16_t x);
static void drawCursor(void);
static void drawPieces(void);
static void drawPiece(int8_t idx);
static bool drawPiecePart(int8_t idx, int8_t x, int8_t y, int8_t c);
static void drawHelp(void);

static bool registerPieces(void);
static void encodePieces(PIECE_T *pPieces, CODE_T *pCode);
static void decodePieces(PIECE_T *pPieces, CODE_T *pCode);
static void rotatePieces(PIECE_T *pPieces, uint8_t rot);
static uint8_t rotatePiece(int8_t idx, uint8_t rot, int8_t vr);
static uint8_t flipPiece(int8_t idx, uint8_t rot);

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
    { 0,  3, 1 }, { 3,  2, 3 }, { 1,  3, 1 }, { 14, 1, 0 }, { 14, 7, 4 },
    { 14, 6, 3 }, { 15, 1, 1 }, { 14, 4, 3 }, { 1,  7, 0 }, { 3,  6, 6 },
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

PROGMEM static const uint8_t imgColumn[7] = { // 7x7
    0x2A, 0x41, 0x00, 0x41, 0x00, 0x41, 0x2A,
};

PROGMEM static const uint8_t imgCursorBody[7] = { // 7x7
    0x00, 0x1E, 0x0E, 0x1E, 0x3A, 0x10, 0x00,
};

PROGMEM static const uint8_t imgCursorFrame[7] = { // 7x7
    0x3F, 0x21, 0x11, 0x21, 0x45, 0x2B, 0x10,
};

PROGMEM static const uint8_t imgHelpIcon[4][7] = { // 7x5 x4
    { 0x1E, 0x1E, 0x1E, 0x1F, 0x09, 0x09, 0x0F },{ 0x1E, 0x12, 0x12, 0x1F, 0x0F, 0x0F, 0x0F },
    { 0x00, 0x04, 0x0E, 0x00, 0x0E, 0x04, 0x00 },{ 0x00, 0x00, 0x0A, 0x1B, 0x0A, 0x00, 0x00 },
};

static STATE_T  state;
static bool     toDrawAll;
static int8_t   cursorX, cursorY;
static int8_t   padX, padY, padRepeatCount;
static int8_t   focusPieceIdx;
static PIECE_T  pieceAry[PIECES];
static int8_t   pieceOrder[PIECES];
static int8_t   board[BOARD_H][BOARD_W];
static int8_t   helpX, helpY;
static bool     isNew;

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
        helpX = HELP_RIGHT_POS;
    }

    putPieces();
    focusPiece(cursorX, cursorY);
    state = STATE_FREE;
    padRepeatCount = 0;
    helpY = HEIGHT;
}

bool updateGame(void)
{
    handleDPad();
    if (state == STATE_FREE) {
        moveCursor(padX, padY);
        playFrames++;
    } else if (state == STATE_PICKED) {
        movePiece(padX, padY);
        playFrames++;
    } else if (state == STATE_CLEAR) {
        if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
            state = STATE_FREE;
            helpY = HEIGHT;
            toDrawAll = true;
        }
    }
    adjustHelpPosition();
    return (state == STATE_LEAVE);
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
    if (state == STATE_CLEAR) {
        arduboy.printEx(0, 0, (isNew) ? F("NEW PATTERN !") : F("ALREADY FOUND"));
    }
    if (isHelpVisible && (state == STATE_FREE || state == STATE_PICKED)) {
        drawHelp();
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
    int8_t const *pPiecePtn = piecePtn[idx];
    int8_t const *pPieceRot = pieceRot[p->rot];
    for (int8_t dy = yStart; dy <= yEnd; dy++) {
        for (int8_t dx = xStart; dx <= xEnd; dx++) {
            int8_t x = p->x + dx * vx + dy * vy * vh;
            int8_t y = p->y - dx * vy + dy * vx * vh;
            ret = f(idx, x, y, pgm_read_byte(pPieceRot + pgm_read_byte(pPiecePtn++))) || ret;
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

static void focusPiece(int8_t x, int8_t y) {
    focusPieceIdx = board[y][x];
    if (focusPieceIdx >= 0) {
        int i = 9;
        while (pieceOrder[i] != focusPieceIdx) {
            i--;
        }
        if (i != 9) {
            while (i < 9) {
                pieceOrder[i] = pieceOrder[i + 1];
                i++;
            }
            pieceOrder[9] = focusPieceIdx;
            putPiece(focusPieceIdx);
        }
    }
}

static void handleDPad(void)
{
    padX = padY = 0;
    if (arduboy.buttonPressed(LEFT_BUTTON | RIGHT_BUTTON | UP_BUTTON | DOWN_BUTTON)) {
        if (++padRepeatCount >= (PAD_REPEAT_DELAY + PAD_REPEAT_INTERVAL)) {
            padRepeatCount = PAD_REPEAT_DELAY;
        }
        if (padRepeatCount == 1 || padRepeatCount == PAD_REPEAT_DELAY) {
            if (arduboy.buttonPressed(LEFT_BUTTON))  padX--;
            if (arduboy.buttonPressed(RIGHT_BUTTON)) padX++;
            if (arduboy.buttonPressed(UP_BUTTON))    padY--;
            if (arduboy.buttonPressed(DOWN_BUTTON))  padY++;
        }
    } else {
        padRepeatCount = 0;
    }
}

static void moveCursor(int8_t vx, int8_t vy)
{
    if (cursorX + vx < 0 || cursorX + vx >= BOARD_W) vx = 0;
    if (cursorY + vy < 0 || cursorY + vy >= BOARD_H) vy = 0;
    if (vx != 0 || vy != 0) {
        cursorX += vx;
        cursorY += vy;
        focusPiece(cursorX, cursorY);
        toDrawAll = true;
    }
    if (arduboy.buttonDown(B_BUTTON) && focusPieceIdx >= 0) {
        playSoundClick();
        state = STATE_PICKED;
        toDrawAll = true;
    } else if (arduboy.buttonDown(A_BUTTON)) {
        saveRecord((uint16_t *)pieceAry);
        state = STATE_LEAVE;
        if (isHelpVisible) {
            toDrawAll = true;
        }
    }
}

static void movePiece(int8_t vx, int8_t vy)
{
    PIECE_T *p = &pieceAry[focusPieceIdx];
    if (arduboy.buttonPressed(A_BUTTON)) {
        int vr = 0;
        if (arduboy.buttonDown(LEFT_BUTTON))  vr--;
        if (arduboy.buttonDown(RIGHT_BUTTON)) vr++;
        if (vr != 0) {
            p->rot = rotatePiece(focusPieceIdx, p->rot, vr);
            toDrawAll = true;
        } else if (arduboy.buttonDown(UP_BUTTON | DOWN_BUTTON)) {
            p->rot = flipPiece(focusPieceIdx, p->rot);
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
            if (putPieces()) {
                state = STATE_FREE;
            } else {
                isNew = registerPieces();
                state = STATE_CLEAR;
            }
            cursorX = p->x;
            cursorY = p->y;
            if (board[p->y][p->x] != focusPieceIdx) {
                cursorY += (board[p->y - 1][p->x] == focusPieceIdx) ? -1 : 1;
            }
            toDrawAll = true;
        }
    }
}

static void adjustHelpPosition(void)
{
    bool toLeft, toRight;
    if (state == STATE_PICKED) {
        int8_t x = pieceAry[focusPieceIdx].x;
        toLeft = (x > 9);
        toRight = (x < 7);
    } else {
        toLeft = (cursorX > 11);
        toRight = (cursorX < 5);
    }
    if (toLeft && helpX > 0 || toRight && helpX == 0) {
        helpX = HELP_RIGHT_POS - helpX;
        helpY = HEIGHT;
        toDrawAll = true;
    }
    if (helpY > HELP_TOP_LIMIT) {
        helpY--;
    }
}

/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

static void drawBoard(void)
{
    drawDottedVLine(31);
    drawDottedVLine(95);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            arduboy.drawBitmap(x * 14 + 39, y * 14 + 7, imgColumn, 7, 7, WHITE);
        }
    }
}

static void drawDottedVLine(int16_t x)
{
    if (x >= 0 && x < WIDTH) {
        buffer_t *p = arduboy.getBuffer() + x;
        for (int i = 0; i < (HEIGHT / 8); i++, p += WIDTH) {
            *p = 0x55;
        }
    }
}

static void drawCursor(void)
{
    int x = cursorX * 7 + 7, y = cursorY * 7 + 2;
    arduboy.drawBitmap(x, y, imgCursorBody, 7, 7, (playFrames & 1) ? WHITE : BLACK);
    arduboy.drawBitmap(x, y, imgCursorFrame, 7, 7, BLACK);
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
    if (idx == focusPieceIdx && (state == STATE_FREE || state == STATE_PICKED)) {
        if (state == STATE_PICKED) {
            bodyCol = (playFrames & 1) ? WHITE : BLACK;
        }
        frameCol = ((playFrames & 7) < 4) ? WHITE : BLACK;
    }
    arduboy.drawBitmap(x * 7 + 4, y * 7, imgPieceBody[c], 7, 7, bodyCol);
    arduboy.drawBitmap(x * 7 + 4, y * 7, imgPieceFrame[c], 7, 7, frameCol);
    return false;
}

static void drawHelp(void)
{
    int iconIdx = 0;
    const __FlashStringHelper *label1, *label2;
    if (state == STATE_PICKED) {
        if (arduboy.buttonPressed(A_BUTTON)) {
            iconIdx = 2;
            label1 = F("ROT");
            label2 = F("FLIP");
        } else {
            label1 = F("HOLD");
            label2 = F("PUT");
        }
    } else {
        label1 = F("MENU");
        label2 = F("PICK");
    }
    arduboy.fillRect2(helpX - 1, helpY - 1, HELP_W + 1, HELP_H + 1, BLACK);
    arduboy.drawBitmap(helpX, helpY, imgHelpIcon[iconIdx], 7, 5, WHITE);
    arduboy.drawBitmap(helpX, helpY + 6, imgHelpIcon[iconIdx + 1], 7, 5, WHITE);
    arduboy.printEx(helpX + 8, helpY, label1);
    arduboy.printEx(helpX + 8, helpY + 6, label2);
}

/*---------------------------------------------------------------------------*/
/*                            Pattern Management                             */
/*---------------------------------------------------------------------------*/

static bool registerPieces(void)
{
    CODE_T code[PIECES], work[PIECES];
    encodePieces(pieceAry, code);
    eepSeek(EEPROM_STORAGE_SPACE_START);
    for (int i = 0; i < clearCount; i++) {
        eepReadBlock(work, PIECES);
        if (code[0].xy == work[0].xy && memcmp(code + 1, work + 1, PIECES - 1) == 0) {
            return false;
        }
    }
    eepWriteBlock(code, PIECES);
    clearCount++;
    return true;
}

static void encodePieces(PIECE_T *pPieces, CODE_T *pCode)
{
    PIECE_T pieceWorkAry[PIECES];
    memcpy(pieceWorkAry, pPieces, sizeof(PIECE_T) * PIECES);
    PIECE_T *p = &pieceWorkAry[0];
    uint8_t rot = p->rot;
    rotatePieces(p, 0);
    p->rot = rot;
    for (int i = 0; i < PIECES; i++, p++, pCode++) {
        pCode->xy = (p->x - 4) / 2 + p->y / 2 * 5;
        pCode->rot = p->rot;
    }
}

static void decodePieces(PIECE_T *pPieces, CODE_T *pCode)
{
    PIECE_T *p = pPieces;
    for (int i = 0; i < PIECES; i++, p++, pCode++) {
        p->x = pCode->xy % 5 * 2 + 4;
        p->y = pCode->xy / 5 * 2;
        p->rot = pCode->rot;
        if (i <= 1) { // white, green
            if (p->rot & 1) {
                p->y++;
            } else {
                p->x++;
            }
        } else if (i >= 3 && i <= 7) { // orange, red, pink, gray, blue
            p->x++;
            p->y++;
        }

    }
    uint8_t rot = pPieces->rot;
    pPieces->rot = 0;
    rotatePieces(pPieces, rot);
}

static void rotatePieces(PIECE_T *pPieces, uint8_t rot)
{
    if ((pPieces->rot ^ rot) & 4) {
        PIECE_T *p = pPieces;
        for (int i = 0; i < PIECES; i++, p++) {
            p->y = 8 - p->y;
            p->rot = flipPiece(i, p->rot);
        }
    }
    while (pPieces->rot != rot) {
        PIECE_T *p = pPieces;
        for (int i = 0; i < PIECES; i++, p++) {
            uint8_t x = p->x;
            p->x = 12 - p->y;
            p->y = x + 4;
            p->rot = rotatePiece(i, p->rot, 1);
        }
    }
}

static uint8_t rotatePiece(int8_t idx, uint8_t rot, int8_t vr)
{
    if (idx == 8) { // brown
        return 0;
    } else {
        return (rot + vr) & 3 | rot & 4;
    }
}

static uint8_t flipPiece(int8_t idx, uint8_t rot)
{
    if (idx == 2) { // purple
        return rot ^ ((~rot & 1) << 1);
    } else if (idx == 7) { // blue
        return 3 - rot;
    } else if (idx == 8) { // brown
        return 0;
    } else {
        return (4 - rot) & 3 | ~rot & 4;
    }
}

