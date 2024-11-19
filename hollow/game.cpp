#include "common.h"

/*  Defines  */

#define PI 3.141592653589793

/*  Typedefs  */

typedef struct {
    uchar   top;
    uchar   bottom;
} COLUMN;

typedef struct {
    uchar   t;
    int8_t  base;
} DEBRIS;

/*  Local Functions  */

static bool isHollow(void);
static void setColumn(COLUMN &newColumn, uchar top, uchar bottom);
static void growColumn(COLUMN &newColumn, bool isSpace, COLUMN &lastColumn);
static int  calcDotY(int i);
static void drawCaveTop(uchar ptn, int x, int height, int edge, int offset);
static void drawCaveBottom(uchar ptn, int x, int height, int edge, int offset);
static void drawPlayer(int x, int y, bool dir, int anim);
static void drawScoreFigure(int x, int y, int value);

/*  Local Variables  */

PROGMEM static const uint8_t imgPlayer[] = {
    0x10, 0x83, 0xAC, 0x69, 0x6D, 0xAD, 0x82, 0x10, // left 0
    0x00, 0x13, 0x6C, 0x69, 0xED, 0xAD, 0x02, 0x20, // left 1
    0x00, 0x23, 0x2C, 0xE9, 0xED, 0x2D, 0x02, 0x20, // left 2
    0x20, 0x06, 0xD8, 0xD2, 0x5A, 0x5A, 0xC4, 0x10, // left 3
    0x10, 0x82, 0xAD, 0x6D, 0x69, 0xAC, 0x83, 0x10, // right 0
    0x20, 0x02, 0xAD, 0xED, 0x69, 0x6C, 0x13, 0x00, // right 1
    0x20, 0x02, 0x2D, 0xED, 0xE9, 0x2C, 0x23, 0x00, // right 2
    0x10, 0xC4, 0x5A, 0x5A, 0xD2, 0xD8, 0x06, 0x20, // right 3
};

PROGMEM static const uint8_t imgCave[] = {
    0x90, 0x06, 0x4E, 0xE4, 0xC0, 0x88, 0x18, 0x99
};

PROGMEM static const uint8_t imgReady[] = { // "Ready?" 64x16
    0x00, 0x00, 0xFC, 0xFC, 0xFC, 0x8C, 0x8C, 0xCC, 0xFC, 0xF8, 0x78, 0x00, 0x80, 0xC0, 0xE0, 0xE0,
    0x60, 0x60, 0xE0, 0xC0, 0x80, 0x00, 0x00, 0xC0, 0x60, 0x60, 0x60, 0x60, 0xE0, 0xC0, 0x80, 0x00,
    0x80, 0xC0, 0xE0, 0xE0, 0xE0, 0xE0, 0xC0, 0xFE, 0xFE, 0xFE, 0x00, 0x20, 0xE0, 0xE0, 0xE0, 0x00,
    0x00, 0xE0, 0xE0, 0xE0, 0x60, 0x00, 0x00, 0x38, 0x1C, 0x1C, 0x9C, 0xFC, 0xFC, 0x78, 0x00, 0x00,
    0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x03, 0x07, 0x1F, 0x3E, 0x3E, 0x38, 0x20, 0x0F, 0x1F, 0x3F, 0x36,
    0x36, 0x36, 0x37, 0x37, 0x07, 0x00, 0x1C, 0x3E, 0x3F, 0x33, 0x33, 0x13, 0x3F, 0x3F, 0x3F, 0x00,
    0x0F, 0x1F, 0x3F, 0x31, 0x30, 0x30, 0x18, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0xC3, 0xCF, 0xFF,
    0xFC, 0x7F, 0x1F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x38, 0x3B, 0x3B, 0x39, 0x00, 0x00, 0x00, 0x00,
};

PROGMEM static const uint8_t imgGameOver[] = { // "GameOver" 88x16
    0x00, 0x00, 0xE0, 0xF0, 0xF8, 0x3C, 0x9C, 0x9C, 0x9C, 0x9C, 0x9C, 0x80, 0x00, 0x00, 0xC0, 0x60,
    0x60, 0x60, 0x60, 0xE0, 0xC0, 0x80, 0x00, 0xE0, 0xE0, 0xE0, 0x40, 0x60, 0x60, 0xE0, 0xE0, 0xC0,
    0x60, 0x60, 0xE0, 0xE0, 0xC0, 0x00, 0x80, 0xC0, 0xE0, 0xE0, 0x60, 0x60, 0xE0, 0xC0, 0x80, 0x00,
    0xE0, 0xF0, 0xF8, 0x3C, 0x1C, 0x1C, 0x1C, 0x3C, 0xF8, 0xF0, 0xE0, 0x00, 0x60, 0xE0, 0xE0, 0xE0,
    0x00, 0x00, 0xE0, 0xE0, 0xE0, 0x60, 0x80, 0xC0, 0xE0, 0xE0, 0x60, 0x60, 0xE0, 0xC0, 0x80, 0x00,
    0xE0, 0xE0, 0xE0, 0xC0, 0xE0, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0F, 0x1F, 0x3C, 0x3B, 0x3B,
    0x3B, 0x3F, 0x3F, 0x1F, 0x00, 0x1C, 0x3E, 0x3F, 0x33, 0x33, 0x13, 0x3F, 0x3F, 0x3F, 0x00, 0x3F,
    0x3F, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x00, 0x0F, 0x1F,
    0x3F, 0x36, 0x36, 0x36, 0x37, 0x37, 0x07, 0x00, 0x07, 0x0F, 0x1F, 0x3C, 0x38, 0x38, 0x38, 0x3C,
    0x1F, 0x0F, 0x07, 0x00, 0x00, 0x03, 0x1F, 0x3F, 0x3E, 0x3E, 0x3F, 0x1F, 0x03, 0x00, 0x0F, 0x1F,
    0x3F, 0x36, 0x36, 0x36, 0x37, 0x37, 0x07, 0x00, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
};

PROGMEM static const uint8_t imgNumber[] = { // 8x16x10
    0xE0, 0xF8, 0xFC, 0x1C, 0x1C, 0xFC, 0xF8, 0xF0, 0x0F, 0x1F, 0x3F, 0x38, 0x38, 0x3F, 0x1F, 0x07,
    0x38, 0x38, 0x38, 0xFC, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x00, 0x00,
    0x00, 0x38, 0x1C, 0x1C, 0x9C, 0xFC, 0xF8, 0x78, 0x38, 0x3C, 0x3E, 0x3B, 0x3B, 0x39, 0x38, 0x38,
    0x00, 0x1C, 0xDC, 0xDC, 0xDC, 0xFC, 0x78, 0x38, 0x00, 0x38, 0x39, 0x39, 0x39, 0x3F, 0x1F, 0x0E,
    0x00, 0xC0, 0xF0, 0x7C, 0xFC, 0xFC, 0xFC, 0x00, 0x0F, 0x0F, 0x0D, 0x0C, 0x3F, 0x3F, 0x3F, 0x0C,
    0xFC, 0xFC, 0xFC, 0xFC, 0xDC, 0xDC, 0x9C, 0x00, 0x39, 0x38, 0x38, 0x38, 0x38, 0x3F, 0x1F, 0x0F,
    0xE0, 0xF0, 0xF8, 0xBC, 0xDC, 0xDC, 0xDC, 0x80, 0x0F, 0x1F, 0x3F, 0x39, 0x39, 0x3F, 0x1F, 0x0F,
    0x1C, 0x1C, 0x1C, 0x9C, 0xDC, 0xFC, 0x7C, 0x1C, 0x00, 0x30, 0x3E, 0x3F, 0x3F, 0x03, 0x00, 0x00,
    0x70, 0xF8, 0xFC, 0x8C, 0x8C, 0xFC, 0xF8, 0x70, 0x0E, 0x1F, 0x3F, 0x31, 0x31, 0x3F, 0x1F, 0x0E,
    0xF0, 0xF8, 0xFC, 0x9C, 0x9C, 0xFC, 0xF8, 0xE0, 0x00, 0x39, 0x39, 0x39, 0x3D, 0x1F, 0x1F, 0x07,
};

PROGMEM static const byte soundStart[] = {
    0x90, 72, 0, 100, 0x80, 0, 25,
    0x90, 74, 0, 100, 0x80, 0, 25,
    0x90, 76, 0, 100, 0x80, 0, 25,
    0x90, 77, 0, 100, 0x80, 0, 25,
    0x90, 79, 0, 200, 0x80, 0xF0
};

PROGMEM static const byte soundMove[] = {
    0x90, 67, 0, 10, 0x90, 79, 0, 10, 0x90, 76, 0, 10, 0x80, 0xF0
};

PROGMEM static const byte soundCrush[] = {
    0x90, 20, 0x91, 19, 1, 244, 0x80, 0x81, 0xF0
};

PROGMEM static const byte soundGameOver[] = {
    0x90, 55, 0, 120, 0x80, 0, 10,
    0x90, 54, 0, 140, 0x80, 0, 20,
    0x90, 53, 0, 160, 0x80, 0, 30,
    0x90, 52, 0, 180, 0x80, 0, 40,
    0x90, 51, 0, 200, 0x80, 0, 50,
    0x90, 50, 0, 220, 0x80, 0, 60,
    0x90, 49, 0, 240, 0x80, 0, 70,
    0x90, 48, 0, 260, 0x80, 0xF0
};

static bool     isStart;
static bool     isOver;
static uint     score;
static uchar    scoreTop;
static bool     isHiscore;
static int      counter;
static uchar    caveOffset;  // 0...143
static int      cavePhase;   // 0...1023
static char     caveHollowCnt;
static int      caveLoopCnt;
static uchar    caveMaxGap;
static int      caveGap;
static int      caveBaseTop;
static int      caveBaseBottom;
static COLUMN   caveColumn[18];
static uchar    playerX;
static uchar    playerColumn;
static uchar    playerMove;  // 0...8
static bool     playerDir;   // false=left / true=right
static char     playerJump;  // -8...8
static DEBRIS   debris[144];

/*---------------------------------------------------------------------------*/

void initGame(void)
{
    isStart = true;
    isOver = false;
    counter = 120; // 2 secs
    score = 0;
    scoreTop = 0;

    caveOffset = 0;
    cavePhase = 0;
    caveHollowCnt = 0;
    caveLoopCnt = 0;
    caveMaxGap = 32;
    for (int i = 0; i < 9; i++) {
        setColumn(caveColumn[i], 32, 40);
    }
    for (int i = 9; i < 18; i++) {
        growColumn(caveColumn[i], isHollow(), caveColumn[i - 1]);
    }
    for (int i = 0; i < 144; i++) {
        debris[i].t = 0;
    }

    playerX = 0;
    playerColumn = 1;
    playerDir = true;
    playerMove = 0;
    playerJump = 0;
    arduboy.playScore2(soundStart, 0);
}

bool updateGame(void)
{
    if (isStart || isOver) {
        if (--counter == 0) isStart = false;
    }
    if (!isStart) {
        cavePhase = (cavePhase + 2 - isOver) % 1024;
    }
    if (cavePhase == 976) {
        arduboy.playScore2(soundCrush, 2);
    }
    caveGap = (0.5 - cos(cavePhase * PI / 512.0) / 2.0) * caveMaxGap;
    caveBaseTop = -(caveGap + 1) / 2;
    caveBaseBottom = caveGap / 2;

    /*  Key handling  */
    if (isOver) {
        if (arduboy.buttonDown(A_BUTTON | B_BUTTON)) {
            initGame();
        }
    } else if (playerMove == 0) {
        int vx = 0;
        if (isStart) {
            vx = 1;
        } else {
            if (arduboy.buttonPressed(LEFT_BUTTON)) vx--;
            if (arduboy.buttonPressed(RIGHT_BUTTON)) vx++;
        }
        if (vx < 0) {
            playerDir = false;
            if (playerX == 0) vx = 0;
        } else if (vx > 0) {
            playerDir = true;
        }
        if (vx != 0) {
            int nextCol = mod(playerColumn + vx, 18);
            int nextGap = min(caveColumn[playerColumn].bottom, caveColumn[nextCol].bottom) -
                    max(caveColumn[playerColumn].top, caveColumn[nextCol].top);
            if (nextGap + caveGap >= 8) {
                playerJump = caveColumn[playerColumn].bottom - caveColumn[nextCol].bottom;
                playerColumn = nextCol;
                playerMove = 8;
                if (playerX + vx > 56) {
                    int growCol = caveOffset / 8;
                    growColumn(caveColumn[growCol], isHollow(), caveColumn[mod(growCol - 1, 18)]);
                    score++;
                }
                if (!isStart) {
                    arduboy.playScore2(soundMove, 3);
                }
            }
        }
    }

    /*  Move and scroll  */
    if (playerMove > 0) {
        playerMove--;
        playerX += playerDir * 2 - 1;
        if (playerX > 56) {
            playerX--;
            caveOffset = (caveOffset + 1) % 144;
        }
    }

    /*  Debris  */
    for (int i = 0, dy; i < 144; i++) {
        if ((i & 7) == 0) dy = caveColumn[i / 8].bottom + caveBaseBottom;
        if (debris[i].t > 0) {
            debris[i].t++;
            if (calcDotY(i) > dy) debris[i].t = 0;
        }
    }
    if (!isStart && rnd((cavePhase >> 6) + 1) == 0) {
        int pos = rnd(144);
        if (debris[pos].t == 0) {
            debris[pos].t = 1;
            debris[pos].base = caveColumn[pos / 8].top + caveBaseTop;
        }
    }

    /*  Judge game over  */
    if (!isStart && cavePhase == 0) {
        caveLoopCnt++;
        if (caveMaxGap < 255) caveMaxGap++;
        if (caveColumn[playerColumn].bottom - caveColumn[playerColumn].top < 4) {
            isOver = true;
            isHiscore = (setLastScore(score, caveLoopCnt * 512) == 0);
            counter = 480; // 8 secs
            arduboy.playScore2(soundGameOver, 1);
        }
    }

    /*  Score displaying  */
    if (playerX < 48 || isStart || isOver) {
        if (scoreTop > 0) scoreTop--;
    } else {
        if (scoreTop < 32) scoreTop++;
    }

    return (isOver && counter == 0);
}

void drawGame(void)
{
    arduboy.clear();

    int shake = (cavePhase > 976) ? cavePhase % 4 / 2 : 0;
    int offsetTop = caveBaseTop + shake;
    int offsetBottom = caveBaseBottom + shake;

    /*  Cave & Debris  */
    for (int i = 0; i < 128; i++) {
        int pos = (i + caveOffset + 8) % 144;
        int col = pos / 8;
        int odd = pos & 7;
        int height, edge; 
        uchar ptn = pgm_read_byte(imgCave + ((i + caveOffset) & 7));

        height = caveColumn[col].top;
        if (odd == 0) {
            edge = height - caveColumn[mod(col - 1, 18)].top + 1;
        } else if (odd == 7) {
            edge = height - caveColumn[mod(col + 1, 18)].top + 1;
        } else {
            edge = 1;
        }
        drawCaveTop(ptn, i, height, edge, -offsetTop);

        height = caveColumn[col].bottom;
        if (odd == 0) {
            edge = caveColumn[mod(col - 1, 18)].bottom - height + 1;
        } else if (odd == 7) {
            edge = caveColumn[mod(col + 1, 18)].bottom - height + 1;
        } else {
            edge = 1;
        }
        drawCaveBottom(ptn, i, 64 - height, edge, offsetBottom);

        if (debris[pos].t > 0) {
            arduboy.drawPixel(i, calcDotY(pos) + shake, WHITE);
        }
    }

    /*  Player  */
    int playerY = caveColumn[playerColumn].bottom + playerJump * playerMove / 8 - 8;
    playerY = max(playerY + offsetBottom, caveColumn[playerColumn].top + offsetTop);
    if (playerY > 56) playerY = 56;
    if (isOver) playerY = caveColumn[playerColumn].top + offsetBottom;
    drawPlayer(playerX, playerY, playerDir, playerMove / 2);

    /*  Score  */
    if (scoreTop > 0) {
        arduboy.setCursor(0, min(scoreTop / 2 - 6, 0));
        arduboy.print(score);
    }

    /*  Message  */
    if (isStart) {
        int y = min(counter - 16, 8);
        arduboy.fillRect2(32, y, 64, 16, BLACK);
        arduboy.drawBitmap(32, y, imgReady, 64, 16, WHITE);
    } else if (isOver) {
        int y = min(464 - counter, 0);
        arduboy.fillRect2(20, y, 88, 16, BLACK);
        arduboy.drawBitmap(20, y, imgGameOver, 88, 16, WHITE);
        y = max(counter - 400, 48);
        arduboy.fillRect2(28, y - isHiscore * 8, 72, 16 + isHiscore * 8, BLACK);
        drawScoreFigure(90, y, score);
        arduboy.printEx(30, y + 3, F("YOUR"));
        arduboy.printEx(30, y + 9, F("SCORE"));
        if (isHiscore && cavePhase % 8 < 4) {
            arduboy.printEx(34, y - 6, F("NEW RECORD"));
        }
    }
}

/*---------------------------------------------------------------------------*/

static bool isHollow(void)
{
    if (caveHollowCnt-- < 0) {
        caveHollowCnt = (rand() + 32768) * score >> 22;
        return true;
    }
    return false;
}

static void setColumn(COLUMN &newColumn, uchar top, uchar bottom)
{
    newColumn.top = top;
    newColumn.bottom = bottom;
}

static void growColumn(COLUMN &newColumn, bool isSpace, COLUMN &lastColumn)
{
    int lastDiff = lastColumn.bottom - lastColumn.top;
    int curDiff = rnd(3);

    if (isSpace) curDiff = 8 - curDiff;
    int adjust = (lastColumn.bottom - 34) / 5;
    int change = rnd(17 - abs(curDiff - lastDiff) - abs(adjust)) - 8;
    if (curDiff > lastDiff) change += curDiff - lastDiff;
    if (adjust < 0) change -= adjust;
    int bottom = lastColumn.bottom + change;
    bottom = min(max(bottom, 16), 56);
    newColumn.top = bottom - curDiff;
    newColumn.bottom = bottom; 
}

static int calcDotY(int i)
{
    return debris[i].base + (debris[i].t * debris[i].t >> 6);
}

static void drawCaveTop(uchar ptn, int x, int height, int edge, int offset)
{
    height -= offset;
    if (height < 0) return;
    edge = min(max(edge, 1), height);
    arduboy.drawFastVLine2(x, height - edge, edge, WHITE);
    height -= edge;

    int shift = -offset & 7;
    ptn = (ptn << shift) | (ptn >> (8 - shift));
    uchar *pBuf = arduboy.getBuffer() + x;
    for (; height > 0; pBuf += WIDTH, height -= 8) {
        uchar mask = (height < 8) ? 0xFF >> (8 - height) : 0xFF;
        *pBuf |= ptn & mask;
    }
}

static void drawCaveBottom(uchar ptn, int x, int height, int edge, int offset)
{
    height -= offset;
    if (height < 0) return;
    edge = min(max(edge, 1), height);
    arduboy.drawFastVLine2(x, 64 - height, edge, WHITE);
    height -= edge;

    int shift = -offset & 7;
    ptn = (ptn >> shift) | (ptn << (8 - shift));
    uchar *pBuf = arduboy.getBuffer() + WIDTH * 7 + x;
    for (; height > 0; pBuf -= WIDTH, height -= 8) {
        uchar mask = (height < 8) ? 0xFF << (8 - height) : 0xFF;
        *pBuf |= ptn & mask;
    }
}

static void drawPlayer(int x, int y, bool dir, int anim)
{
    arduboy.drawBitmap(x, y, imgPlayer + (dir * 4 + anim) * 8, 8, 8, WHITE);
}

static void drawScoreFigure(int x, int y, int value)
{
    if (value > 9) {
        drawScoreFigure(x - 10, y, value / 10);
    }
    arduboy.drawBitmap(x, y, imgNumber + (value % 10) * 16, 8, 16, WHITE);
}
