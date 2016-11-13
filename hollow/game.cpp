#include "common.h"

/*  Defines  */

#define PI 3.141592653589793

#define rnd(val)        (rand() % (val))
#define mod(value, div) (((value) + div) % div)

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

static bool isHollow();
static void setColumn(COLUMN &newColumn, uchar top, uchar bottom);
static void growColumn(COLUMN &newColumn, bool isSpace, COLUMN &lastColumn);
static int  calcDotY(int i);
static void drawCaveTop(uchar ptn, int x, int height, int edge, int offset);
static void drawCaveBottom(uchar ptn, int x, int height, int edge, int offset);
static void drawPlayer(int x, int y, bool dir, int anim);

/*  Local Variables  */

PROGMEM static const uint8_t imgPlayer[] = {
    0x10, 0x86, 0x89, 0x39, 0x3F, 0xBF, 0x86, 0x10, // left 0
    0x00, 0x16, 0x49, 0x39, 0xBF, 0xBF, 0x06, 0x20, // left 1
    0x00, 0x26, 0x09, 0xB9, 0xBF, 0x3F, 0x06, 0x20, // left 2
    0x20, 0x0C, 0x92, 0xB2, 0x3E, 0x3E, 0xCC, 0x10, // left 3
    0x10, 0x86, 0xBF, 0x3F, 0x39, 0x89, 0x86, 0x10, // right 0
    0x20, 0x06, 0xBF, 0xBF, 0x39, 0x49, 0x16, 0x00, // right 1
    0x20, 0x06, 0x3F, 0xBF, 0xB9, 0x09, 0x26, 0x00, // right 2
    0x10, 0xCC, 0x3E, 0x3E, 0xB2, 0x92, 0x0C, 0x20, // right 3
};
PROGMEM static const uint8_t imgCave[] = {
    //0x3C, 0x3D, 0xBB, 0x9B, 0xDB, 0xC1, 0xCD, 0xDC
    0x3C, 0x3C, 0x99, 0x93, 0xC3, 0xC1, 0x88, 0x1C,
};

static bool     isStart;
static bool     isOver;
static int      score;
static int      counter;
static uchar    caveOffset;  // 0...143
static int      cavePhase;   // 0...1023
static char     caveHollowCnt;
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

void initGame()
{
    isStart = true;
    isOver = false;
    counter = 120; // 2 secs
    score = 0;

    caveOffset = 0;
    cavePhase = 0;
    caveHollowCnt = 0;
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
}

bool updateGame()
{
    if (isStart || isOver) {
        if (--counter == 0) isStart = false;
    }
    if (!isStart) {
        cavePhase = (cavePhase + 2 - isOver) % 1024;
    }
    caveGap = (0.5 - cos(cavePhase * PI / 512.0) / 2.0) * caveMaxGap;
    caveBaseTop = -(caveGap + 1) / 2;
    caveBaseBottom = caveGap / 2;

    /*  Key handling  */
    if (isOver) {
        if (arduboy.pressed(B_BUTTON)) {
            initGame();
        }
    } else if (playerMove == 0) {
        int vx = 0;
        if (isStart) {
            vx = 1;
        } else {
            if (arduboy.pressed(LEFT_BUTTON)) vx--;
            if (arduboy.pressed(RIGHT_BUTTON)) vx++;
        }
        if (vx < 0) {
            playerDir = false;
            if (playerX == 0) vx = 0;
        } else if (vx > 0) {
            playerDir = true;
        }
        if (vx != 0) {
            int nextCol = mod(playerColumn + vx, 18);
            int nextGap = min(caveColumn[playerColumn].bottom - caveColumn[nextCol].top,
                    caveColumn[nextCol].bottom - caveColumn[playerColumn].top);
            if (nextGap + caveGap >= 8) {
                playerJump = caveColumn[playerColumn].bottom - caveColumn[nextCol].bottom;
                playerColumn = nextCol;
                playerMove = 8;
                if (playerX + vx > 56) {
                    int growCol = caveOffset / 8;
                    growColumn(caveColumn[growCol], isHollow(), caveColumn[mod(growCol - 1, 18)]);
                    score++;
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
        if (caveColumn[playerColumn].bottom - caveColumn[playerColumn].top < 4) {
            isOver = true;
            counter = 480; // 8 secs
        }
        caveMaxGap++;
    }

    return (isOver && counter == 0);
}

void drawGame()
{
    arduboy.clear();

    int shake = (cavePhase > 976) ? cavePhase % 4 / 2: 0;
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
    if (!isStart) {
        arduboy.setCursor(0, 0);
        arduboy.print(score);
    }

    /*  Message  */
    if (isStart) {
        arduboy.setCursor(46, 24);
        arduboy.print(F("READY?"));
    } else if (isOver) {
        arduboy.setCursor(37, 24);
        arduboy.print(F("GAME OVER"));
    }
}

/*---------------------------------------------------------------------------*/

static bool isHollow()
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
    arduboy.drawFastVLine(x, height - edge, edge, WHITE);
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
    arduboy.drawFastVLine(x, 64 - height, edge, WHITE);
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
