#include "common.h"

/*  Defines  */

#define BOARD_W 17
#define BOARD_H 9

#define EMPTY   -1
#define COLUMN  -2

#define FRAMES_5SECS    (60 * 5)
#define FRAMES_30SECS   (60 * 20)
#define FRAMES_2MINS    (60 * 60 * 2)

enum STATE_T {
    STATE_INIT = 0,
    STATE_FREE,
    STATE_PICKED,
    STATE_CLEAR,
    STATE_LEAVE,
};

/*  Typedefs  */

typedef struct {
    int8_t  idx:7;
    int8_t  isCorner:1;
} BOARD_T;

typedef struct {
    uint8_t iconIdx1;
    char    label1[5];
    uint8_t iconIdx2;
    char    label2[5];
} HELPLBL_T;

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
static void saveAndResetCreep(void);

static void drawDottedVLine(int16_t x);
static void drawCursor(void);
static void drawPieces(void);
static bool drawPiecePart(int8_t idx, int8_t x, int8_t y, int8_t c);
static void drawClearEffect(void);

static uint8_t checkAndRegisterPieces(void);
static void encodePieces(CODE_T *pCode);
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

PROGMEM static const uint8_t imgClear[216] = { // "A New Pattern" 108x16
    0x00, 0x00, 0x00, 0x00, 0xE0, 0x18, 0x3C, 0xF8, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x0C, 0xFC, 0x38, 0x70, 0xE0, 0xC0, 0x00, 0x04, 0x04, 0xFC, 0x04, 0x04, 0x00, 0x80,
    0x40, 0x40, 0xC0, 0x80, 0x00, 0x40, 0xC0, 0xC0, 0x40, 0x00, 0x40, 0xC0, 0xC0, 0x40, 0x00, 0x40,
    0xC0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0xFC, 0xFC, 0x04, 0x04, 0x04, 0x8C, 0xF8,
    0x70, 0x00, 0x80, 0xC0, 0x40, 0x40, 0xC0, 0x80, 0x00, 0x40, 0xE0, 0xF8, 0x40, 0x40, 0x40, 0xE0,
    0xF8, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x40, 0xC0, 0x80, 0x00, 0x80, 0xC0, 0xC0, 0x80, 0x40,
    0xC0, 0x00, 0x80, 0xC0, 0xC0, 0x80, 0x40, 0x40, 0xC0, 0x80, 0x00, 0x00, 0x20, 0x30, 0x3C, 0x23,
    0x02, 0x02, 0x02, 0x23, 0x3F, 0x3E, 0x30, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x3F,
    0x20, 0x20, 0x00, 0x01, 0x07, 0x0E, 0x1C, 0x3F, 0x00, 0x00, 0x0F, 0x1F, 0x39, 0x31, 0x31, 0x19,
    0x00, 0x00, 0x01, 0x07, 0x1E, 0x38, 0x0C, 0x03, 0x07, 0x1E, 0x38, 0x0E, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x20, 0x20, 0x3F, 0x3F, 0x21, 0x21, 0x01, 0x01, 0x00, 0x00, 0x00, 0x39, 0x3D,
    0x24, 0x22, 0x3F, 0x3F, 0x20, 0x00, 0x1F, 0x3F, 0x20, 0x10, 0x00, 0x1F, 0x3F, 0x20, 0x10, 0x00,
    0x0F, 0x1F, 0x39, 0x31, 0x31, 0x19, 0x00, 0x20, 0x3F, 0x3F, 0x20, 0x00, 0x00, 0x00, 0x20, 0x3F,
    0x3F, 0x20, 0x00, 0x20, 0x3F, 0x3F, 0x20, 0x00,
};

PROGMEM static const uint8_t imgHelpIcon[4][7] = { // 7x5 x4
    { 0x1E, 0x1E, 0x1E, 0x1F, 0x09, 0x09, 0x0F },{ 0x1E, 0x12, 0x12, 0x1F, 0x0F, 0x0F, 0x0F },
    { 0x00, 0x04, 0x0E, 0x00, 0x0E, 0x04, 0x00 },{ 0x00, 0x00, 0x0A, 0x1B, 0x0A, 0x00, 0x00 },
};

PROGMEM static const HELPLBL_T helpAry[] = {
    { 0, "MENU", 1, "PICK" }, { 0, "HOLD", 1, "PUT"  },
    { 2, "ROT",  3, "FLIP" }, { 2, "PAGE", 0, "MENU" },
};

PROGMEM static const byte soundPick[] = {
    0x90, 65, 0, 40, 0x90, 69, 0, 40, 0x90, 72, 0, 40, 0x80, 0xF0
};

PROGMEM static const byte soundPut[] = {
    0x90, 72, 0, 40, 0x90, 69, 0, 40, 0x90, 65, 0, 40, 0x80, 0xF0
};

PROGMEM static const byte soundRotate[] = {
    0x90, 88, 0, 10, 0x90, 86, 0, 10, 0x90, 84, 0, 10,
    0x90, 82, 0, 10, 0x90, 80, 0, 10, 0x80, 0xF0
};

PROGMEM static const byte soundFlip[] = {
    0x90, 70, 0, 10, 0x90, 80, 0, 10, 0x90, 75, 0, 10,
    0x90, 85, 0, 10, 0x90, 80, 0, 10, 0x80, 0xF0
};

PROGMEM static const byte soundNewPattern[] = {
    0x90, 81, 0, 40, 0x80, 0, 40,
    0x90, 86, 0, 40, 0x80, 0, 40,
    0x90, 90, 0, 40, 0x80, 0, 40, 0xE0
};

PROGMEM static const byte soundExistedPattern[] = {
    0x90, 60, 0, 64, 0x80, 0, 64, 0xE0
};

static STATE_T  state = STATE_INIT;
static bool     toDrawAll;
static int8_t   cursorX, cursorY;
static int8_t   focusPieceIdx;
static int8_t   pieceOrder[PIECES];
static BOARD_T  board[BOARD_H][BOARD_W];
static uint16_t creepFrames, quietFrames;
static uint8_t  lastPatternIdx;
static uint8_t  clearEffectCount;
static bool     isNew;

/*---------------------------------------------------------------------------*/
/*                              Main Functions                               */
/*---------------------------------------------------------------------------*/

void initPuzzle(void)
{
    if (state == STATE_INIT) {
        for (int i = 0; i < PIECES; i++) {
            pieceOrder[i] = i;
        }
        cursorX = 8;
        cursorY = 4;
        helpX = HELP_RIGHT_POS;
        creepFrames = 0;
        lastPatternIdx = 255; // trick!
        dprintln(F("Initialize puzzle"));
    }

    putPieces();
    focusPiece(cursorX, cursorY);
    state = STATE_FREE;
    padRepeatCount = 0;
    helpY = HEIGHT;
    if (!isDirty) {
        creepFrames = 0;
    }
    quietFrames = 0;
}

MODE_T updatePuzzle(void)
{
    handleDPad();
    if (state == STATE_FREE) {
        moveCursor(padX, padY);
        playFrames++;
    } else if (state == STATE_PICKED) {
        movePiece(padX, padY);
        playFrames++;
    } else if (state == STATE_CLEAR) {
        clearEffectCount--;
        if (clearEffectCount == 0) {
            arduboy.stopScore2();
            state = STATE_FREE;
            helpY = HEIGHT;
            toDrawAll = true;
        }
    }
    adjustHelpPosition();

    if (creepFrames < FRAMES_2MINS) {
        creepFrames++;
    }
    if (padRepeatCount == 0) {
        if (quietFrames < FRAMES_5SECS) {
            quietFrames++;
        } else if (isDirty && creepFrames >= FRAMES_2MINS) {
            saveAndResetCreep();
            dprintln(F("Auto save"));
        }
    } else {
        quietFrames = 0;
    }
    return (state == STATE_LEAVE) ? MODE_MENU : MODE_PUZZLE;
}

void drawPuzzle(void)
{
    bool isPlaying = (state == STATE_FREE || state == STATE_PICKED);
    if (toDrawAll) {
        arduboy.clear();
        drawBoard(0);
        drawPieces();
        toDrawAll = false;
    } else if (isPlaying && focusPieceIdx >= 0) {
        drawPiece(focusPieceIdx);
    }

    if (state == STATE_FREE) {
        drawCursor();
    }
    if (state == STATE_CLEAR) {
        drawClearEffect();
    }
    if (isHelpVisible && isPlaying) {
        HELP_T idx;
        if (state == STATE_FREE) {
            idx = HELP_FREE;
        } else {
            idx = (arduboy.buttonPressed(A_BUTTON)) ? HELP_HOLD : HELP_PICK;
        }
        drawHelp(idx, helpX, helpY);
    }
}

/*---------------------------------------------------------------------------*/
/*                             Control Functions                             */
/*---------------------------------------------------------------------------*/

void resetPieces(void)
{
    memcpy_P(pieceAry, pieceAryDefault, sizeof(PIECE_T) * PIECES);
    dprintln(F("Reset pieces"));
}

static bool putPieces(void)
{
    for (int y = 0; y < BOARD_H; y++) {
        for (int x = 0; x < BOARD_W; x++) {
            board[y][x].idx = (x < 4 || x > 12 || (x & y & 1)) ? COLUMN : EMPTY;
            board[y][x].isCorner = false;
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
            ret = (!board[y][x].isCorner && board[y][x].idx != EMPTY);
            board[y][x].idx = idx;
            board[y][x].isCorner = (c < 5);
        }
    }
    return ret;
}

static void focusPiece(int8_t x, int8_t y) {
    focusPieceIdx = board[y][x].idx;
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
        arduboy.playScore2(soundPick, 2);
        state = STATE_PICKED;
        toDrawAll = true;
        dprint(F("Pick piece="));
        dprintln(focusPieceIdx);
    } else if (arduboy.buttonDown(A_BUTTON)) {
        playSoundClick();
        if (isDirty && creepFrames >= FRAMES_30SECS) {
            saveAndResetCreep();
        }
        state = STATE_LEAVE;
        toDrawAll = true;
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
            arduboy.playScore2(soundRotate, 3);
            p->rot = rotatePiece(focusPieceIdx, p->rot, vr);
            isDirty = true;
            toDrawAll = true;
        } else if (arduboy.buttonDown(UP_BUTTON | DOWN_BUTTON)) {
            arduboy.playScore2(soundFlip, 3);
            p->rot = flipPiece(focusPieceIdx, p->rot);
            isDirty = true;
            toDrawAll = true;
        }
        if (toDrawAll) {
            dprint(F("Rotate rot="));
            dprintln(p->rot);
        }
    } else {
        int g = (focusPieceIdx < 3) ? 0 : 1;
        if (p->x + vx < g || p->x + vx >= BOARD_W - g) vx = 0;
        if (p->y + vy < g || p->y + vy >= BOARD_H - g) vy = 0;
        if (vx != 0 || vy != 0) {
            playSoundTick();
            p->x += vx;
            p->y += vy;
            isDirty = true; 
            toDrawAll = true;
        }
        if (arduboy.buttonDown(B_BUTTON)) {
            if (putPieces()) {
                arduboy.playScore2(soundPut, 2);
                state = STATE_FREE;
                dprintln(F("Release"));
            } else {
                uint8_t idx = checkAndRegisterPieces();
                isNew = (idx == clearCount);
                if (idx == lastPatternIdx) {
                    arduboy.playScore2(soundPut, 2);
                    state = STATE_FREE;
                } else {
                    lastPatternIdx = idx;
                    if (isNew) {
                        arduboy.playScore2(soundNewPattern, 1);
                        clearCount++;
                        saveAndResetCreep();
                        setGalleryIndex(idx);
                        clearEffectCount = 120;
                    } else {
                        arduboy.playScore2(soundExistedPattern, 1);
                        clearEffectCount = 60;
                    }
                    state = STATE_CLEAR;
                }
                dprint(F("Completed! isNew="));
                dprintln(isNew);
            }
            cursorX = p->x;
            cursorY = p->y;
            if (board[p->y][p->x].idx != focusPieceIdx) {
                cursorY += (board[p->y - 1][p->x].idx == focusPieceIdx) ? -1 : 1;
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

static void saveAndResetCreep(void)
{
    writeRecord();
    creepFrames = 0;
}


/*---------------------------------------------------------------------------*/
/*                              Draw Functions                               */
/*---------------------------------------------------------------------------*/

void drawBoard(int16_t offsetX)
{
    drawDottedVLine(31 + offsetX);
    drawDottedVLine(95 + offsetX);
    offsetX += 39;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            arduboy.drawBitmap(x * 14 + offsetX, y * 14 + 7, imgColumn, 7, 7, WHITE);
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
        drawPiece(pieceOrder[i]);
    }
}

void drawPiece(int8_t idx)
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

static void drawClearEffect(void)
{
    if (isNew) {
        if (clearEffectCount > 104) {
            int h = 121 - clearEffectCount;
            int y = (64 - h) / 2;
            arduboy.fillRect(0, y, 128, h, WHITE);
        } else {
            int d, v;
            if (clearEffectCount >= 88) {
                d = clearEffectCount - 88;
                v = 1;
            } else if (clearEffectCount <= 16) {
                d = 17 - clearEffectCount;
                v = -1;
            } else {
                return;
            }
            arduboy.fillRect(0, 24, 128, 16, WHITE);
            arduboy.drawBitmap(10 + d * (d + 1) / 2 * v, 24, imgClear, 108, 16, BLACK);
        }
    } else {
        if (clearEffectCount > 51) {
            int h = 61 - clearEffectCount;
            int y = (64 - h) / 2;
            arduboy.fillRect(0, y, 128, h, BLACK);
            arduboy.drawRect(-1, y, 130, h, WHITE);
        } else if (clearEffectCount == 51) {
            arduboy.printEx(25, 29, F("ALREADY FOUND"));
        }
    }
}

void drawHelp(HELP_T idx, int16_t x, int16_t y)
{
    uint8_t *p = (uint8_t *) &helpAry[idx];
    arduboy.fillRect2(x - 1, y - 1, HELP_W + 1, HELP_H + 1, BLACK);
    arduboy.drawBitmap(x, y,     imgHelpIcon[pgm_read_byte(p)],     7, 5, WHITE);
    arduboy.drawBitmap(x, y + 6, imgHelpIcon[pgm_read_byte(p + 6)], 7, 5, WHITE);
    arduboy.printEx(x + 8, y,     (const __FlashStringHelper *) (p + 1));
    arduboy.printEx(x + 8, y + 6, (const __FlashStringHelper *) (p + 7));
}

/*---------------------------------------------------------------------------*/
/*                       Completed Pattern Management                        */
/*---------------------------------------------------------------------------*/

static uint8_t checkAndRegisterPieces(void)
{
    CODE_T code[PIECES], work[PIECES];
    encodePieces(code);
    dprintln(F("Checking history..."));
    for (int i = -1; i < clearCount; i++) {
        uint8_t idx = (i == -1) ? lastPatternIdx : i;
        if (idx < clearCount && i != lastPatternIdx) {
            readEncodedPieces(idx, work);
            if (code[0].xy == work[0].xy && memcmp(code + 1, work + 1, PIECES - 1) == 0) {
                return idx;
            }
        }
    }
    writeEncodedPieces(clearCount, code);
    return clearCount;
}

static void encodePieces(CODE_T *pCode)
{
    PIECE_T pieceWorkAry[PIECES];
    memcpy(pieceWorkAry, pieceAry, sizeof(PIECE_T) * PIECES);
    uint8_t rot = pieceWorkAry[0].rot;
    PIECE_T *p = pieceWorkAry;
    rotatePieces(p, 0);
    pieceWorkAry[0].rot = rot;
    for (int i = 0; i < PIECES; i++, p++, pCode++) {
        pCode->xy = (p->x - 4) / 2 + p->y / 2 * 5;
        pCode->rot = p->rot;
    }
    dprintln(F("Encoded pieces"));
}

void decodePieces(CODE_T *pCode)
{
    PIECE_T *p = pieceAry;
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
    uint8_t rot = pieceAry[0].rot;
    pieceAry[0].rot = 0;
    rotatePieces(pieceAry, rot);
    dprintln(F("Decoded pieces"));
}

static void rotatePieces(PIECE_T *pPieces, uint8_t rot)
{
    if ((pPieces[0].rot ^ rot) & 4) {
        PIECE_T *p = pPieces;
        for (int i = 0; i < PIECES; i++, p++) {
            p->y = 8 - p->y;
            p->rot = flipPiece(i, p->rot);
        }
    }
    while (pPieces[0].rot != rot) {
        PIECE_T *p = pPieces;
        for (int i = 0; i < PIECES; i++, p++) {
            uint8_t x = p->x;
            p->x = 12 - p->y;
            p->y = x - 4;
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

